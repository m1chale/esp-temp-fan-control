
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "dht.h"
#include <mqtt_client.h>
#include "mqtt.h"
#include "wifi.h"
#include "nvs_flash.h"
#include "pwm.h"
#include <math.h>

#define PWM_GPIO CONFIG_PWM_GPIO
#define DHT_GPIO CONFIG_DHT_GPIO
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASS
#define MQTT_URI CONFIG_MQTT_URI
#define MQTT_USERNAME CONFIG_MQTT_USERNAME
#define MQTT_PASSWORD CONFIG_MQTT_PASSWORD
#define MQTT_TOPIC_TEMPERATURE "sensors/serverrack/temperature"
#define MQTT_TOPIC_FAN_SPEED "sensors/serverrack/fanspeed"
#define REFRESH_INTERVAL_MINS 5

void init_nvs(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

float get_temperature(void)
{
    float temperature = 0, humidity = 0;
    esp_err_t result = dht_read_float_data(DHT_TYPE_AM2301, DHT_GPIO, &humidity, &temperature);

    if (result == ESP_OK)
    {
        ESP_LOGI("DHT", "Temp: %.1f°C | Humidity: %.1f%%", temperature, humidity);
        return temperature;
    }
    else
    {
        ESP_LOGE("DHT", "Failed to read: %s", esp_err_to_name(result));
        return NAN;
    }
}

void publishData(esp_mqtt_client_handle_t mqtt_client, float temperature, uint8_t fan_speed_percent)
{
    char temp_payload[16];
    snprintf(temp_payload, sizeof(temp_payload), "%.1f", temperature);
    esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_TEMPERATURE, temp_payload, 0, 1, 0);

    char fanspeed_payload[8];
    snprintf(fanspeed_payload, sizeof(fanspeed_payload), "%d", fan_speed_percent);
    esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_FAN_SPEED, fanspeed_payload, 0, 1, 0);
}

uint8_t map_percent_to_8bit(uint8_t percent)
{
    return (uint8_t)ceil((float)percent * 255.0f / 100.0f);
}

uint8_t map_8bit_to_percent(uint8_t bit)
{
    return bit * 100 / 255;
}

uint8_t calc_pwm_duty(float temperature, float last_temperature)
{
    // secure pwm range for fan is around 50% - 100%
    if (temperature < 25)
        return 0;
    if (temperature < 27 && (last_temperature < 27 || temperature < last_temperature - 0.5))
        return map_percent_to_8bit(50);
    if (temperature < 30 && (last_temperature < 30 || temperature < last_temperature - 0.5))
        return map_percent_to_8bit(65);
    if (temperature < 32 && (last_temperature < 32 || temperature < last_temperature - 0.5))
        return map_percent_to_8bit(80);

    return map_percent_to_8bit(100);
}

uint8_t calc_pwm_duty_fine(float temperature)
{
    // secure pwm range for fan is around 50% - 100%

    const float lowerTempBound = 25.0f;
    const float upperTempBound = 30.0f;
    const int minSpeedPercent = 50;
    const int maxSpeedPercent = 100;

    if (temperature < lowerTempBound)
        return map_percent_to_8bit(0);
    if (temperature >= upperTempBound)
        return map_percent_to_8bit(maxSpeedPercent);

    // linear interpolation between two points (x0, y0) and (x1, y1):
    // y = y0 + (x - x0) * (y1 - y0) / (x1 - x0)
    // So the fan speed increases linearly from minSpeedPercent at lowerTempBound up to maxSpeedPercent at upperTempBound.
    float slope = (maxSpeedPercent - minSpeedPercent) / (upperTempBound - lowerTempBound);
    int fanSpeedPercent = (int)(minSpeedPercent + (temperature - lowerTempBound) * slope);
    return map_percent_to_8bit(fanSpeedPercent);
}

void init(void)
{
    init_pwm(PWM_GPIO);
    init_nvs();
    wifi_init_sta(WIFI_SSID, WIFI_PASS);
    vTaskDelay(pdMS_TO_TICKS(5000)); // wait for 5 seconds until wifi comes up
}

void app_main(void)
{
    init();
    esp_mqtt_client_handle_t mqtt_client = mqtt_app_start(MQTT_URI, MQTT_USERNAME, MQTT_PASSWORD);

    float last_temp = 0.0f;
    float temp = 0.0f;

    while (true)
    {
        temp = get_temperature();
        if (isnan(temp))
        {
            ESP_LOGW("TEMP", "Skipping fan update due to failed sensor read");
            continue;
        }

        uint8_t pwm_duty = calc_pwm_duty_fine(temp);
        last_temp = temp;

        set_pwm_duty(pwm_duty);
        uint8_t pwm_duty_percent = map_8bit_to_percent(pwm_duty);
        ESP_LOGI("PWM", "Duty: %u | %u%%", pwm_duty, pwm_duty_percent);

        publishData(mqtt_client, temp, pwm_duty_percent);

        vTaskDelay(pdMS_TO_TICKS(1000 * 60 * REFRESH_INTERVAL_MINS)); // wait interval
    }
}

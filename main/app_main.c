
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

#define DHT_GPIO CONFIG_DHT_GPIO
#define WIFI_SSID CONFIG_WIFI_SSID
#define WIFI_PASS CONFIG_WIFI_PASS
#define MQTT_URI CONFIG_MQTT_URI
#define MQTT_USERNAME CONFIG_MQTT_USERNAME
#define MQTT_PASSWORD CONFIG_MQTT_PASSWORD

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

float get_and_publish_temperature(esp_mqtt_client_handle_t mqtt_client)
{
    float temperature = 0, humidity = 0;
    esp_err_t result = dht_read_float_data(DHT_TYPE_AM2301, DHT_GPIO, &humidity, &temperature);

    if (result == ESP_OK)
    {
        ESP_LOGI("DHT", "Temp: %.1f°C | Humidity: %.1f%%", temperature, humidity);

        // publish temperature via mqtt
        char temp_payload[16];
        snprintf(temp_payload, sizeof(temp_payload), "%.1f", temperature);
        esp_mqtt_client_publish(mqtt_client, "sensors/serverrack/temperature", temp_payload, 0, 1, 0);

        char fanspeed_payload[8];
        snprintf(fanspeed_payload, sizeof(fanspeed_payload), "%d", 0); // Currently hardcoded to 0
        esp_mqtt_client_publish(mqtt_client, "sensors/serverrack/fanspeed", fanspeed_payload, 0, 1, 0);

        return temperature;
    }
    else
    {
        ESP_LOGE("DHT", "Failed to read: %s", esp_err_to_name(result));
        return 0.0f;
    }
}

uint8_t map_percent_to_8bit(uint8_t percent)
{
    return percent * 255 / 100;
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

void init(void)
{
    init_pwm();
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
        temp = get_and_publish_temperature(mqtt_client);
        uint8_t pwm_duty = calc_pwm_duty(temp, last_temp);

        set_pwm_duty(pwm_duty);
        ESP_LOGI("PWM", "Duty: %u | %u%%", pwm_duty, map_8bit_to_percent(pwm_duty));

        last_temp = temp;
        vTaskDelay(pdMS_TO_TICKS(1000 * 60 * 1)); // wait a minute
    }

    // TODO
    // pwm richtig initialisieren
    // photo hochladen
    // alle daten bereitstellen
}

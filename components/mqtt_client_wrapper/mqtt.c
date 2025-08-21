#include "mqtt_client.h"
#include "esp_log.h"

static const char *LOG_TAG = "MQTT";
static esp_mqtt_client_handle_t mqtt_client = NULL;

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    // esp_mqtt_event_handle_t event = event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(LOG_TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(LOG_TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(LOG_TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        break;
    }
}

esp_mqtt_client_handle_t mqtt_app_start(const char *uri, const char *username, const char *password)
{
    if (mqtt_client != NULL)
    {
        ESP_LOGW(LOG_TAG, "MQTT client already initialized");
        return mqtt_client;
    }

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = uri,
        .credentials.username = username,
        .credentials.authentication.password = password};

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (mqtt_client == NULL)
    {
        ESP_LOGE(LOG_TAG, "Failed to initialize MQTT client");
        return NULL;
    }

    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);

    return mqtt_client;
}
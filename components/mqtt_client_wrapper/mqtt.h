#ifndef MQTT_H
#define MQTT_H

#include <mqtt_client.h>

esp_mqtt_client_handle_t mqtt_app_start(const char *uri, const char *username, const char *password);

#endif
idf_component_register(SRCS "app_main.c"
                       PRIV_REQUIRES spi_flash wifi mqtt_client_wrapper dht nvs_flash esp_event pwm
                       INCLUDE_DIRS "")


idf_component_register(SRCS "mqtt.c"
                      INCLUDE_DIRS "."
                      PRIV_REQUIRES esp_event
                      REQUIRES mqtt)
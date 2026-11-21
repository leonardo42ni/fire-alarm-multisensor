idf_component_register(
    SRCS "net_mqtt.c"
    INCLUDE_DIRS "include"
    REQUIRES common esp_wifi esp_netif esp_event mbedtls mqtt nvs_flash log
)

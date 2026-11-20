idf_component_register(
    SRCS "zb_coord.c"
    INCLUDE_DIRS "include"
    REQUIRES common espressif__esp-zigbee-lib freertos esp_common log
)

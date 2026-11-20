idf_component_register(
    SRCS "uart_bridge.c"
    INCLUDE_DIRS "include"
    REQUIRES common zb_coord espressif__esp-zigbee-lib esp_driver_uart esp_timer esp_system freertos log
)

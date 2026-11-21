idf_component_register(
    SRCS "uart_link.c"
    INCLUDE_DIRS "include"
    REQUIRES common esp_driver_uart esp_timer esp_system freertos log
)

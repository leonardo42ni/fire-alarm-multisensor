idf_component_register(
    SRCS "nodes_map.c" "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES common esp_driver_gpio esp_system esp_timer freertos log uart_link
)

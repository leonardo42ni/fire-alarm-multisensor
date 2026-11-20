idf_component_register(
    SRCS "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES common esp_system log nvs_flash zb_coord uart_bridge
)

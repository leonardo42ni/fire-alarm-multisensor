idf_component_register(
    SRCS "app_main.c" "node_zb_ep.c"
    INCLUDE_DIRS "."
    REQUIRES common esp_system log nvs_flash esp_timer
)

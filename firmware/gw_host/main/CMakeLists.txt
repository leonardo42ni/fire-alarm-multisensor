idf_component_register(
    SRCS "nodes_map.c" "app_main.c"
    INCLUDE_DIRS "."
    REQUIRES common esp_system log
)

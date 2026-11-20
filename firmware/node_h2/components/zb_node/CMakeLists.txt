idf_component_register(
    SRCS "zb_node.c" "zb_node_stack.c" "zb_node_ep.c"
    INCLUDE_DIRS "include"
    REQUIRES common espressif__esp-zigbee-lib esp_timer log
)

#ifndef ZB_NODE_INTERNAL_H
#define ZB_NODE_INTERNAL_H

#include <stdbool.h>

#include "esp_err.h"
#include "esp_zigbee_core.h"

void zb_node_task(void *arg);
esp_err_t zb_node_create_endpoints(esp_zb_ep_list_t **out_ep_list);
void zb_node_set_joined(bool joined);
void zb_node_request_reset_app(void);

#endif

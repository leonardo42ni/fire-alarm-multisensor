#ifndef NODE_ZB_EP_H
#define NODE_ZB_EP_H

#include "esp_err.h"
#include "esp_zigbee_core.h"

esp_err_t node_zb_create_endpoints(esp_zb_ep_list_t **out_ep_list);

#endif

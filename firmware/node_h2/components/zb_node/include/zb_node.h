#ifndef ZB_NODE_H
#define ZB_NODE_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

/* DEV_CODING_PLAN §5.3.6 snapshot; T-106 will own this definition. */
typedef struct {
    uint32_t t_ms;
    float temp;
    float hum;
    uint16_t eco2;
    uint16_t tvoc;
    float mq2_mv;
    float flame_mv;
    bool flame_do;
    bool prealert;
    uint16_t flags;
    uint32_t seq;
} node_snapshot_t;

esp_err_t zb_node_start(void);
bool zb_node_joined(void);
esp_err_t zb_node_publish(const node_snapshot_t *snap);

#endif

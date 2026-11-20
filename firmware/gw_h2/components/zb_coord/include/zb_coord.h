#ifndef ZB_COORD_H
#define ZB_COORD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "proto_ndjson.h"

#define ZB_COORD_TASK_STACK 8192
#define ZB_COORD_TASK_PRIO 5
#define ZB_COORD_QUEUE_LEN 32
#define ZB_COORD_LOCK_TIMEOUT_MS 1000u
#define ZB_COORD_RESET_DELAY_MS 1000u
#define ZB_COORD_MAX_NODES 4
#define ZB_COORD_INVALID_SA 0xFFFFu

typedef enum {
    ZB_EVT_JOIN = 1,
    ZB_EVT_LEAVE,
    ZB_EVT_REPORT
} zb_evt_type_t;

typedef struct {
    zb_evt_type_t type;
    uint16_t sa;
    char ieee[PROTO_IEEE_LEN];
    uint8_t ep;
    uint16_t cluster;
    uint16_t attr;
    uint8_t attr_type;
    union {
        int16_t s16;
        uint16_t u16;
        uint32_t u32;
        float f32;
    } val;
} zb_evt_t;

esp_err_t zb_coord_start(void);
QueueHandle_t zb_coord_evt_queue(void);
uint8_t zb_coord_node_count(void);
bool zb_coord_ieee_for(uint16_t sa, char *out, size_t cap);
esp_err_t zb_coord_open_network(uint8_t seconds);
esp_err_t zb_coord_reset_network(void);

#endif

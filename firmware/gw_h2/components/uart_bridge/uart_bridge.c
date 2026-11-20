#include "uart_bridge.h"
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "zcl/esp_zigbee_zcl_analog_input.h"
#include "zcl/esp_zigbee_zcl_common.h"
#include "zcl/esp_zigbee_zcl_humidity_meas.h"
#include "zcl/esp_zigbee_zcl_temperature_meas.h"
#include "proto_ndjson.h"
#include "shared_config.h"
#include "version.h"
#include "zb_coord.h"
typedef struct { bool used; uint16_t sa; rpt_t rpt; bool pending; uint32_t first_ms; } node_slot_t;
static const char *TAG = "BRG";
static QueueHandle_t s_queue;
static line_asm_t s_line;
static node_slot_t s_slots[UART_BRIDGE_MAX_NODES];
static bool s_started;
static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / (int64_t)UART_BRIDGE_MS_PER_S); }
static void uart_send(const char *line, int len) { if (len > 0) (void)uart_write_bytes(UART_NUM_1, line, (size_t)len); }
static node_slot_t *slot_find(uint16_t sa)
{
    for (int i = 0; i < UART_BRIDGE_MAX_NODES; i++) {
        if (s_slots[i].used && s_slots[i].sa == sa) return &s_slots[i];
    }
    return NULL;
}
static node_slot_t *slot_acquire(uint16_t sa)
{
    node_slot_t *slot = slot_find(sa);
    if (slot != NULL) return slot;
    for (int i = 0; i < UART_BRIDGE_MAX_NODES; i++) {
        if (s_slots[i].used) continue;
        slot = &s_slots[i];
        memset(slot, 0, sizeof(*slot));
        slot->used = true;
        slot->sa = sa;
        slot->rpt.sa = sa;
        if (!zb_coord_ieee_for(sa, slot->rpt.ieee, PROTO_IEEE_LEN)) slot->rpt.ieee[0] = '\0';
        return slot;
    }
    return NULL;
}
static void slot_reset(node_slot_t *s) { s->pending = false; s->rpt.valid = 0; s->rpt.seq = 0; }
static void flush_slot(node_slot_t *slot)
{
    char line[PROTO_MAX_LINE];
    if (slot->rpt.ieee[0] == '\0') (void)zb_coord_ieee_for(slot->sa, slot->rpt.ieee, PROTO_IEEE_LEN);
    int n = proto_enc_rpt(line, sizeof(line), &slot->rpt);
    if (n > 0) {
        uart_send(line, n);
        ESP_LOGI(TAG, "BU|ZB_BRIDGE|RPT|SA=0x%04x|seq=%lu", slot->sa, (unsigned long)slot->rpt.seq);
    } else {
        ESP_LOGW(TAG, "BU|ZB_BRIDGE|RPT|FAIL|%d", n);
    }
    slot_reset(slot);
}
static bool map_analog(const zb_evt_t *e, float *dst, uint32_t bit, rpt_t *r)
{
    if (e->cluster != ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT ||
        e->attr != ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID ||
        e->attr_type != ESP_ZB_ZCL_ATTR_TYPE_SINGLE) return false;
    *dst = e->val.f32;
    r->valid |= bit;
    return true;
}
static bool map_report(node_slot_t *slot, const zb_evt_t *e)
{
    rpt_t *r = &slot->rpt;
    switch (e->ep) {
    case EP_TEMP:
        if (e->cluster == ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT &&
            e->attr == ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID &&
            e->attr_type == ESP_ZB_ZCL_ATTR_TYPE_S16) {
            r->temp = (float)e->val.s16 / ZCL_CENTI_SCALE;
            r->valid |= RPT_HAS_TEMP;
            return true;
        }
        if (e->cluster == ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT &&
            e->attr == ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID &&
            e->attr_type == ESP_ZB_ZCL_ATTR_TYPE_U16) {
            r->hum = (float)e->val.u16 / ZCL_CENTI_SCALE;
            r->valid |= RPT_HAS_HUM;
            return true;
        }
        return false;
    case EP_MQ2: return map_analog(e, &r->mq2_mv, RPT_HAS_MQ2, r);
    case EP_FLAME: return map_analog(e, &r->flame_mv, RPT_HAS_FLAME, r);
    case EP_ECO2: return map_analog(e, &r->eco2, RPT_HAS_ECO2, r);
    case EP_TVOC: return map_analog(e, &r->tvoc, RPT_HAS_TVOC, r);
    case EP_FLAGS:
        if (e->cluster == ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT &&
            e->attr == ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID &&
            e->attr_type == ESP_ZB_ZCL_ATTR_TYPE_SINGLE) {
            if (!isfinite(e->val.f32) || e->val.f32 < 0.0f || e->val.f32 > (float)UINT16_MAX) return false;
            r->flags = (uint16_t)e->val.f32;
            r->valid |= RPT_HAS_FLAGS;
            return true;
        }
        if (e->cluster == ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT &&
            e->attr == ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID &&
            e->attr_type == ESP_ZB_ZCL_ATTR_TYPE_U32) {
            r->seq = e->val.u32;
            return true;
        }
        return false;
    default: return false;
    }
}
static void on_join(const zb_evt_t *e)
{
    if (e->ieee[0] != '\0') {
        for (int i = 0; i < UART_BRIDGE_MAX_NODES; i++) {
            if (s_slots[i].used && s_slots[i].sa != e->sa &&
                strncmp(s_slots[i].rpt.ieee, e->ieee, PROTO_IEEE_LEN) == 0) {
                memset(&s_slots[i], 0, sizeof(s_slots[i]));
            }
        }
    }
    node_slot_t *slot = slot_acquire(e->sa);
    char line[PROTO_MAX_LINE];
    if (slot == NULL) return;
    if (e->ieee[0] != '\0') memcpy(slot->rpt.ieee, e->ieee, PROTO_IEEE_LEN);
    slot_reset(slot);
    uart_send(line, proto_enc_join(line, sizeof(line), slot->rpt.ieee, slot->sa));
}
static void on_leave(const zb_evt_t *e)
{
    node_slot_t *slot = slot_find(e->sa);
    char line[PROTO_MAX_LINE];
    const char *ieee = (slot != NULL && slot->rpt.ieee[0] != '\0') ? slot->rpt.ieee : e->ieee;
    uart_send(line, proto_enc_leave(line, sizeof(line), ieee, e->sa));
    if (slot != NULL) memset(slot, 0, sizeof(*slot));
}
static void on_report(const zb_evt_t *e)
{
    node_slot_t *slot = slot_acquire(e->sa);
    bool seq = e->ep == EP_FLAGS && e->cluster == ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT &&
               e->attr == ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID && e->attr_type == ESP_ZB_ZCL_ATTR_TYPE_U32;
    if (slot == NULL) return;
    if (slot->rpt.ieee[0] == '\0' && e->ieee[0] != '\0') memcpy(slot->rpt.ieee, e->ieee, PROTO_IEEE_LEN);
    if (!map_report(slot, e)) return;
    if (!slot->pending) { slot->pending = true; slot->first_ms = now_ms(); }
    if (seq) flush_slot(slot);
}
static void check_timeouts(uint32_t now)
{
    for (int i = 0; i < UART_BRIDGE_MAX_NODES; i++) {
        if (s_slots[i].used && s_slots[i].pending && (now - s_slots[i].first_ms) >= COALESCE_MS) flush_slot(&s_slots[i]);
    }
}
static void send_hb(uint32_t now) { char line[PROTO_MAX_LINE]; uart_send(line, proto_enc_hb(line, sizeof(line), now / UART_BRIDGE_MS_PER_S, zb_coord_node_count())); }
static void handle_cmd(const proto_msg_t *msg)
{
    char line[PROTO_MAX_LINE];
    switch (msg->u.cmd.op) {
    case OP_PING:
        uart_send(line, proto_enc_ack(line, sizeof(line), OP_PING));
        break;
    case OP_PERMIT_JOIN: {
        uint16_t sec = msg->u.cmd.sec;
        if (sec < UART_BRIDGE_PERMIT_MIN_S) sec = UART_BRIDGE_PERMIT_MIN_S;
        if (sec > UART_BRIDGE_PERMIT_MAX_S) sec = UART_BRIDGE_PERMIT_MAX_S;
        esp_err_t err = zb_coord_open_network((uint8_t)sec);
        ESP_LOGI(TAG, "BU|ZB_BRIDGE|PERMIT|sec=%u|%s", sec, esp_err_to_name(err));
        break;
    }
    case OP_RESET_NET: {
        esp_err_t err = zb_coord_reset_network();
        if (err != ESP_OK) ESP_LOGW(TAG, "BU|ZB_BRIDGE|RESET|FAIL|%s", esp_err_to_name(err));
        else { memset(s_slots, 0, sizeof(s_slots)); ESP_LOGW(TAG, "BU|ZB_BRIDGE|RESET"); }
        break;
    }
    default: break;
    }
}
static void on_line(const char *line, size_t len, void *ctx) { proto_msg_t msg; (void)ctx; if (proto_dec(line, len, &msg) == PROTO_OK && msg.type == M_CMD) handle_cmd(&msg); }
static void bridge_task(void *arg)
{
    (void)arg;
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    uint8_t rx[UART_BRIDGE_RX_CHUNK];
    uint32_t last_hb = now_ms();
    for (;;) {
        zb_evt_t evt;
        if (xQueueReceive(s_queue, &evt, pdMS_TO_TICKS(UART_BRIDGE_TICK_MS)) == pdTRUE) {
            if (evt.type == ZB_EVT_JOIN) on_join(&evt);
            else if (evt.type == ZB_EVT_LEAVE) on_leave(&evt);
            else if (evt.type == ZB_EVT_REPORT) on_report(&evt);
        }
        int n;
        while ((n = uart_read_bytes(UART_NUM_1, rx, sizeof(rx), 0)) > 0) line_asm_feed(&s_line, rx, (size_t)n, on_line, NULL);
        uint32_t now = now_ms();
        check_timeouts(now);
        if (now - last_hb >= HB_INTERVAL_MS) { last_hb = now; send_hb(now); }
        esp_task_wdt_reset();
    }
}
esp_err_t uart_bridge_start(void)
{
    if (s_started) return ESP_ERR_INVALID_STATE;
    if ((s_queue = zb_coord_evt_queue()) == NULL) return ESP_ERR_INVALID_STATE;
    const uart_config_t cfg = {
        .baud_rate = UART_BAUD, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(UART_NUM_1, &cfg);
    if (err != ESP_OK) return err;
    err = uart_set_pin(UART_NUM_1, UART_BRIDGE_TX_GPIO, UART_BRIDGE_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err != ESP_OK) return err;
    err = uart_driver_install(UART_NUM_1, UART_RX_BUF, UART_TX_BUF, 0, NULL, 0);
    if (err != ESP_OK) return err;
    line_asm_init(&s_line);
    if (xTaskCreate(bridge_task, "uart_bridge", UART_BRIDGE_TASK_STACK, NULL, UART_BRIDGE_TASK_PRIO, NULL) != pdPASS) {
        (void)uart_driver_delete(UART_NUM_1);
        return ESP_ERR_NO_MEM;
    }
    s_started = true;
    char line[PROTO_MAX_LINE];
    uart_send(line, proto_enc_hello(line, sizeof(line), FW_VERSION, ZB_CHANNEL, ZB_PAN_ID));
    ESP_LOGI(TAG, "BU|ZB_BRIDGE|START|UART1|TX=%d|RX=%d", UART_BRIDGE_TX_GPIO, UART_BRIDGE_RX_GPIO);
    return ESP_OK;
}

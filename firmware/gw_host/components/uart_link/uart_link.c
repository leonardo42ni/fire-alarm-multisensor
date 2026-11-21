#include "uart_link.h"

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/task.h"

#define UART_LINK_FMT_BUF 20

typedef struct {
    bool used;
    bool online;
    uint16_t sa;
    uint32_t last_seen_ms;
    char ieee[PROTO_IEEE_LEN];
} node_entry_t;

static const char *TAG = "LNK";
static QueueHandle_t s_q_rpt;
static line_asm_t s_line;
static node_entry_t s_nodes[UART_LINK_MAX_NODES];
static bool s_started;
static atomic_bool s_h2_online = ATOMIC_VAR_INIT(false);
static atomic_bool s_any_node_online = ATOMIC_VAR_INIT(false);
static bool s_h2_offline_logged;
static uint32_t s_last_hb_ms;

static uint32_t now_ms(void) { return (uint32_t)(esp_timer_get_time() / (int64_t)UART_LINK_MS_PER_S); }

static void refresh_any_online(void)
{
    bool any = false;
    for (int i = 0; i < UART_LINK_MAX_NODES; i++) {
        if (s_nodes[i].used && s_nodes[i].online) { any = true; break; }
    }
    atomic_store(&s_any_node_online, any);
}

static void mark_h2_alive(uint32_t now)
{
    s_last_hb_ms = now;
    s_h2_offline_logged = false;
    if (!atomic_load(&s_h2_online)) {
        atomic_store(&s_h2_online, true);
        ESP_LOGI(TAG, "BU|UART_LINK|H2_ONLINE");
    }
}

static node_entry_t *find_slot(const char *ieee, uint16_t sa, bool allow_alloc)
{
    int free_idx = -1, reuse_idx = -1;
    uint32_t oldest_ms = UINT32_MAX;

    if (ieee != NULL && ieee[0] != '\0') {
        for (int i = 0; i < UART_LINK_MAX_NODES; i++) {
            if (s_nodes[i].used && strncmp(s_nodes[i].ieee, ieee, PROTO_IEEE_LEN) == 0) return &s_nodes[i];
        }
    }
    for (int i = 0; i < UART_LINK_MAX_NODES; i++) {
        if (s_nodes[i].used && s_nodes[i].sa == sa) return &s_nodes[i];
        if (!s_nodes[i].used && free_idx < 0) free_idx = i;
        else if (s_nodes[i].used && !s_nodes[i].online && s_nodes[i].last_seen_ms <= oldest_ms) {
            oldest_ms = s_nodes[i].last_seen_ms;
            reuse_idx = i;
        }
    }
    if (!allow_alloc) return NULL;
    int idx = (free_idx >= 0) ? free_idx : reuse_idx;
    if (idx < 0) return NULL;
    memset(&s_nodes[idx], 0, sizeof(s_nodes[idx]));
    return &s_nodes[idx];
}

static void node_upsert(const char *ieee, uint16_t sa, uint32_t now, bool online)
{
    node_entry_t *e = find_slot(ieee, sa, online);
    if (e == NULL) {
        if (online) ESP_LOGW(TAG, "BU|UART_LINK|NODE_FULL|sa=0x%04x", (unsigned)sa);
        return;
    }
    bool was_online = e->used && e->online;
    e->used = true;
    e->online = online;
    e->sa = sa;
    e->last_seen_ms = now;
    if (ieee != NULL && ieee[0] != '\0') snprintf(e->ieee, sizeof(e->ieee), "%s", ieee);
    const char *id = (e->ieee[0] != '\0') ? e->ieee : "unknown";
    if (online && !was_online) ESP_LOGI(TAG, "BU|UART_LINK|NODE_ONLINE|ieee=%s|sa=0x%04x", id, (unsigned)e->sa);
    else if (!online && was_online) ESP_LOGW(TAG, "BU|UART_LINK|NODE_LEAVE|ieee=%s|sa=0x%04x", id, (unsigned)e->sa);
    refresh_any_online();
}

static void on_line(const char *line, size_t len, void *ctx)
{
    (void)ctx;
    proto_msg_t msg;
    proto_err_t err = proto_dec(line, len, &msg);
    if (err != PROTO_OK) { ESP_LOGW(TAG, "BU|UART_LINK|DEC_ERR|%d", (int)err); return; }

    uint32_t now = now_ms();
    mark_h2_alive(now);
    switch (msg.type) {
    case M_HELLO:
        ESP_LOGI(TAG, "BU|UART_LINK|HELLO|fw=%s|ch=%u|pan=0x%04x",
                 msg.u.hello.fw, (unsigned)msg.u.hello.ch, (unsigned)msg.u.hello.pan);
        break;
    case M_HB:
        ESP_LOGI(TAG, "BU|UART_LINK|HB|up=%lu|nodes=%u",
                 (unsigned long)msg.u.hb.up, (unsigned)msg.u.hb.nodes);
        break;
    case M_ACK:
        ESP_LOGI(TAG, "BU|UART_LINK|ACK|op=%d", (int)msg.u.ack.op);
        break;
    case M_JOIN:
        node_upsert(msg.u.join.ieee, msg.u.join.sa, now, true);
        ESP_LOGI(TAG, "BU|UART_LINK|JOIN|ieee=%s|sa=0x%04x",
                 msg.u.join.ieee, (unsigned)msg.u.join.sa);
        break;
    case M_LEAVE:
        node_upsert(msg.u.leave.ieee, msg.u.leave.sa, now, false);
        break;
    case M_RPT:
        node_upsert(msg.u.rpt.ieee, msg.u.rpt.sa, now, true);
        if (xQueueSend(s_q_rpt, &msg.u.rpt, 0) != pdTRUE) {
            ESP_LOGW(TAG, "BU|UART_LINK|Q_FULL|seq=%lu", (unsigned long)msg.u.rpt.seq);
        }
        break;
    default: break;
    }
}

static void check_timeouts(uint32_t now)
{
    if ((now - s_last_hb_ms) >= HB_TIMEOUT_MS &&
        (atomic_load(&s_h2_online) || !s_h2_offline_logged)) {
        atomic_store(&s_h2_online, false);
        s_h2_offline_logged = true;
        ESP_LOGW(TAG, "BU|UART_LINK|H2_OFFLINE");
    }
    bool changed = false;
    for (int i = 0; i < UART_LINK_MAX_NODES; i++) {
        node_entry_t *e = &s_nodes[i];
        if (e->used && e->online && (now - e->last_seen_ms) >= UART_LINK_NODE_TIMEOUT_MS) {
            e->online = false;
            changed = true;
            ESP_LOGW(TAG, "BU|UART_LINK|NODE_OFFLINE|ieee=%s|sa=0x%04x",
                     e->ieee[0] ? e->ieee : "unknown", (unsigned)e->sa);
        }
    }
    if (changed) refresh_any_online();
}

static void link_task(void *arg)
{
    (void)arg;
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    uint8_t rx[UART_LINK_RX_CHUNK];
    uint32_t prev_ovf = 0;
    s_last_hb_ms = now_ms();

    for (;;) {
        int n = uart_read_bytes(UART_NUM_2, rx, sizeof(rx), pdMS_TO_TICKS(UART_LINK_READ_TIMEOUT_MS));
        if (n > 0) {
            line_asm_feed(&s_line, rx, (size_t)n, on_line, NULL);
            if (s_line.n_overflow != prev_ovf) {
                prev_ovf = s_line.n_overflow;
                ESP_LOGW(TAG, "BU|UART_LINK|LINE_OVF|%lu", (unsigned long)prev_ovf);
            }
        }
        check_timeouts(now_ms());
        esp_task_wdt_reset();
    }
}

esp_err_t uart_link_start(void)
{
    if (s_started) return ESP_ERR_INVALID_STATE;
    if ((s_q_rpt = xQueueCreate(UART_LINK_Q_RPT_LEN, sizeof(rpt_t))) == NULL) return ESP_ERR_NO_MEM;

    const uart_config_t cfg = {
        .baud_rate = UART_BAUD, .data_bits = UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, .source_clk = UART_SCLK_DEFAULT,
    };
    esp_err_t err = uart_param_config(UART_NUM_2, &cfg);
    if (err == ESP_OK) err = uart_set_pin(UART_NUM_2, UART_LINK_TX_GPIO, UART_LINK_RX_GPIO, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (err == ESP_OK) err = uart_driver_install(UART_NUM_2, UART_RX_BUF, UART_TX_BUF, 0, NULL, 0);
    if (err != ESP_OK) { vQueueDelete(s_q_rpt); s_q_rpt = NULL; return err; }

    line_asm_init(&s_line);
    memset(s_nodes, 0, sizeof(s_nodes));
    atomic_store(&s_h2_online, false);
    atomic_store(&s_any_node_online, false);
    s_h2_offline_logged = false;

    if (xTaskCreate(link_task, "uart_link", UART_LINK_TASK_STACK, NULL, UART_LINK_TASK_PRIO, NULL) != pdPASS) {
        (void)uart_driver_delete(UART_NUM_2);
        vQueueDelete(s_q_rpt);
        s_q_rpt = NULL;
        return ESP_ERR_NO_MEM;
    }
    s_started = true;
    ESP_LOGI(TAG, "BU|UART_LINK|START|UART2|RX=%d|TX=%d", UART_LINK_RX_GPIO, UART_LINK_TX_GPIO);
    return ESP_OK;
}

QueueHandle_t uart_link_rpt_queue(void) { return s_q_rpt; }
bool uart_link_h2_online(void) { return atomic_load(&s_h2_online); }
bool uart_link_any_node_online(void) { return atomic_load(&s_any_node_online); }

esp_err_t uart_link_send_cmd(cmd_op_t op, uint16_t sec)
{
    if (!s_started) return ESP_ERR_INVALID_STATE;
    char line[PROTO_MAX_LINE];
    int n = proto_enc_cmd(line, sizeof(line), op, sec);
    if (n <= 0) return ESP_ERR_INVALID_ARG;
    return (uart_write_bytes(UART_NUM_2, line, (size_t)n) == n) ? ESP_OK : ESP_FAIL;
}

static void fmt_opt(char *dst, size_t cap, bool ok, float v, const char *fmt)
{
    if (!ok || !isfinite(v)) { dst[0] = '\0'; return; }
    snprintf(dst, cap, fmt, v);
}

void uart_link_print_csv(const rpt_t *r, uint32_t t_ms)
{
    if (r == NULL) return;
    char t[UART_LINK_FMT_BUF], h[UART_LINK_FMT_BUF], c[UART_LINK_FMT_BUF];
    char v[UART_LINK_FMT_BUF], m[UART_LINK_FMT_BUF], f[UART_LINK_FMT_BUF];
    fmt_opt(t, sizeof(t), (r->valid & RPT_HAS_TEMP) != 0u, r->temp, "%.2f");
    fmt_opt(h, sizeof(h), (r->valid & RPT_HAS_HUM) != 0u, r->hum, "%.2f");
    fmt_opt(c, sizeof(c), (r->valid & RPT_HAS_ECO2) != 0u, r->eco2, "%.0f");
    fmt_opt(v, sizeof(v), (r->valid & RPT_HAS_TVOC) != 0u, r->tvoc, "%.0f");
    fmt_opt(m, sizeof(m), (r->valid & RPT_HAS_MQ2) != 0u, r->mq2_mv, "%.1f");
    fmt_opt(f, sizeof(f), (r->valid & RPT_HAS_FLAME) != 0u, r->flame_mv, "%.1f");

    unsigned flags = ((r->valid & RPT_HAS_FLAGS) != 0u) ? (unsigned)r->flags : 0u;
    const char *node = (r->ieee[0] != '\0') ? r->ieee : "unknown";
    char out[PROTO_MAX_LINE];
    int n = snprintf(out, sizeof(out), "CSV,%lu,%s,%lu,%s,%s,%s,%s,%s,%s,%u,%d,%u,0,0,0,0,0,0,0.0,0\n",
                     (unsigned long)t_ms, node, (unsigned long)r->seq, t, h, c, v, m, f, flags, (int)r->rssi, (unsigned)r->lqi);
    if (n > 0 && n < (int)sizeof(out)) { fputs(out, stdout); fflush(stdout); }
}

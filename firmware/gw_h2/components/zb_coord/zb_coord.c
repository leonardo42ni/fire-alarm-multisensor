#include "zb_coord.h"
#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "shared_config.h"
#define ESP_ZB_ZC_CONFIG() { .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR, .install_code_policy = ZB_INSTALL_CODE_POLICY, .nwk_cfg.zczr_cfg = { .max_children = ZB_COORD_MAX_NODES } }
#define ESP_ZB_DEFAULT_RADIO_CONFIG() { .radio_mode = ZB_RADIO_MODE_NATIVE }
#define ESP_ZB_DEFAULT_HOST_CONFIG() { .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE }
typedef struct { bool used; uint16_t sa; char ieee[PROTO_IEEE_LEN]; } node_entry_t;
static const char *TAG = "ZB";
static QueueHandle_t s_queue;
static bool s_started;
static node_entry_t s_nodes[ZB_COORD_MAX_NODES];
static portMUX_TYPE s_mux = portMUX_INITIALIZER_UNLOCKED;
static void format_ieee(const esp_zb_ieee_addr_t ieee, char *out)
{
    snprintf(out, PROTO_IEEE_LEN, "%02x%02x%02x%02x%02x%02x%02x%02x",
             ieee[7], ieee[6], ieee[5], ieee[4], ieee[3], ieee[2], ieee[1], ieee[0]);
}
static void cache_set(uint16_t sa, const char *ieee, bool used)
{
    int target = -1;
    if (used && ieee == NULL) return;
    taskENTER_CRITICAL(&s_mux);
    for (int i = 0; i < ZB_COORD_MAX_NODES; i++) {
        if (s_nodes[i].used &&
            (s_nodes[i].sa == sa || (ieee != NULL && strncmp(s_nodes[i].ieee, ieee, PROTO_IEEE_LEN) == 0))) {
            s_nodes[i].used = false;
            if (target < 0) target = i;
        } else if (!s_nodes[i].used && target < 0) {
            target = i;
        }
    }
    if (used && target >= 0) {
        s_nodes[target] = (node_entry_t){ .used = true, .sa = sa };
        memcpy(s_nodes[target].ieee, ieee, PROTO_IEEE_LEN);
    }
    taskEXIT_CRITICAL(&s_mux);
}
static bool cache_get(uint16_t sa, char *out)
{
    bool found = false;
    taskENTER_CRITICAL(&s_mux);
    for (int i = 0; i < ZB_COORD_MAX_NODES; i++) {
        if (s_nodes[i].used && s_nodes[i].sa == sa) {
            if (out != NULL) memcpy(out, s_nodes[i].ieee, PROTO_IEEE_LEN);
            found = true; break;
        }
    }
    taskEXIT_CRITICAL(&s_mux);
    return found;
}
uint8_t zb_coord_node_count(void)
{
    uint8_t count = 0;
    taskENTER_CRITICAL(&s_mux);
    for (int i = 0; i < ZB_COORD_MAX_NODES; i++) { if (s_nodes[i].used) count++; }
    taskEXIT_CRITICAL(&s_mux);
    return count;
}
bool zb_coord_ieee_for(uint16_t sa, char *out, size_t cap)
{
    esp_zb_ieee_addr_t raw;
    if (out == NULL || cap < PROTO_IEEE_LEN) return false;
    if (cache_get(sa, out)) return true;
    if (!esp_zb_lock_acquire(pdMS_TO_TICKS(ZB_COORD_LOCK_TIMEOUT_MS))) return false;
    esp_err_t err = esp_zb_ieee_address_by_short(sa, raw);
    esp_zb_lock_release();
    if (err != ESP_OK) return false;
    format_ieee(raw, out);
    cache_set(sa, out, true);
    return true;
}
static void push_event(zb_evt_type_t type, uint16_t sa, const char *ieee)
{
    zb_evt_t evt = { .type = type, .sa = sa };
    if (s_queue == NULL) return;
    if (ieee != NULL) memcpy(evt.ieee, ieee, PROTO_IEEE_LEN);
    (void)xQueueSend(s_queue, &evt, 0);
}
static void handle_node_leave(uint16_t sa, const esp_zb_ieee_addr_t addr)
{
    char ieee[PROTO_IEEE_LEN];
    format_ieee(addr, ieee);
    cache_set(sa, ieee, false);
    push_event(ZB_EVT_LEAVE, sa, ieee);
    ESP_LOGW(TAG, "BU|ZB_COORD|LEAVE|SA=0x%04x", sa);
}
static void push_report(const esp_zb_zcl_report_attr_message_t *report)
{
    zb_evt_t evt = {0};
    const void *v = report->attribute.data.value;
    uint16_t sz = report->attribute.data.size;
    esp_zb_ieee_addr_t raw;
    bool resolved = false;
    if (s_queue == NULL || v == NULL) return;
    if (report->src_address.addr_type == ESP_ZB_ZCL_ADDR_TYPE_SHORT) {
        evt.sa = report->src_address.u.short_addr;
        resolved = !cache_get(evt.sa, evt.ieee) && esp_zb_ieee_address_by_short(evt.sa, raw) == ESP_OK;
    } else if (report->src_address.addr_type == ESP_ZB_ZCL_ADDR_TYPE_IEEE) {
        memcpy(raw, report->src_address.u.ieee_addr, sizeof(raw));
        evt.sa = esp_zb_address_short_by_ieee(raw);
        if (evt.sa == ZB_COORD_INVALID_SA) return;
        resolved = true;
    } else { return; }
    if (resolved) { format_ieee(raw, evt.ieee); cache_set(evt.sa, evt.ieee, true); }
    evt.type = ZB_EVT_REPORT;
    evt.ep = report->src_endpoint;
    evt.cluster = report->cluster;
    evt.attr = report->attribute.id;
    evt.attr_type = report->attribute.data.type;
    switch (evt.attr_type) {
    case ESP_ZB_ZCL_ATTR_TYPE_S16:    if (sz != sizeof(int16_t))  return; memcpy(&evt.val.s16, v, sizeof(int16_t));  break;
    case ESP_ZB_ZCL_ATTR_TYPE_U16:    if (sz != sizeof(uint16_t)) return; memcpy(&evt.val.u16, v, sizeof(uint16_t)); break;
    case ESP_ZB_ZCL_ATTR_TYPE_U32:    if (sz != sizeof(uint32_t)) return; memcpy(&evt.val.u32, v, sizeof(uint32_t)); break;
    case ESP_ZB_ZCL_ATTR_TYPE_SINGLE: if (sz != sizeof(float))    return; memcpy(&evt.val.f32, v, sizeof(float));    break;
    default: return;
    }
    (void)xQueueSend(s_queue, &evt, 0);
}
static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t id, const void *message)
{
    const esp_zb_zcl_report_attr_message_t *r = message;
    if (id == ESP_ZB_CORE_REPORT_ATTR_CB_ID && r != NULL && r->status == ESP_ZB_ZCL_STATUS_SUCCESS) push_report(r);
    return ESP_OK;
}
static void bdb_start_cb(uint8_t mode_mask)
{
    esp_err_t err = esp_zb_bdb_start_top_level_commissioning(mode_mask);
    if (err != ESP_OK) ESP_LOGW(TAG, "BDB start failed: %s", esp_err_to_name(err));
}
static void open_join_window(void)
{
    ESP_LOGI(TAG, "BU|ZB_COORD|FORMED|PAN=0x%04x|CH=%d|SA=0x%04x|PASS", esp_zb_get_pan_id(), esp_zb_get_current_channel(), esp_zb_get_short_address());
    esp_err_t err = esp_zb_bdb_open_network(JOIN_WINDOW_S);
    if (err == ESP_OK) ESP_LOGI(TAG, "BU|ZB_COORD|PERMIT|OPEN|sec=%u", JOIN_WINDOW_S);
    else ESP_LOGW(TAG, "BU|ZB_COORD|PERMIT|FAIL|%s", esp_err_to_name(err));
}
static void reset_local_cb(uint8_t param) { (void)param; esp_zb_bdb_reset_via_local_action(); }
esp_err_t zb_coord_open_network(uint8_t seconds)
{
    if (!esp_zb_lock_acquire(pdMS_TO_TICKS(ZB_COORD_LOCK_TIMEOUT_MS))) return ESP_ERR_TIMEOUT;
    esp_err_t err = esp_zb_bdb_open_network(seconds);
    esp_zb_lock_release();
    return err;
}
esp_err_t zb_coord_reset_network(void)
{
    if (!esp_zb_lock_acquire(pdMS_TO_TICKS(ZB_COORD_LOCK_TIMEOUT_MS))) return ESP_ERR_TIMEOUT;
    esp_zb_scheduler_alarm(reset_local_cb, 0, 0);
    esp_zb_lock_release();
    return ESP_OK;
}
static esp_err_t create_endpoint(esp_zb_ep_list_t **out)
{
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_cluster_list_t *cl = esp_zb_zcl_cluster_list_create();
    esp_zb_basic_cluster_cfg_t basic = { ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE, ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE };
    esp_zb_attribute_list_t *b_attr = (ep_list != NULL && cl != NULL) ? esp_zb_basic_cluster_create(&basic) : NULL;
    if (b_attr == NULL) return ESP_ERR_NO_MEM;
    esp_err_t err = esp_zb_cluster_list_add_basic_cluster(cl, b_attr, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (err != ESP_OK) return err;
    esp_zb_endpoint_config_t cfg = { COORD_EP, ESP_ZB_AF_HA_PROFILE_ID, ESP_ZB_HA_HOME_GATEWAY_DEVICE_ID, 0 };
    if ((err = esp_zb_ep_list_add_gateway_ep(ep_list, cl, cfg)) != ESP_OK) return err;
    *out = ep_list;
    return ESP_OK;
}
static void zb_task(void *arg)
{
    (void)arg;
    esp_zb_platform_config_t platform = { .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(), .host_config = ESP_ZB_DEFAULT_HOST_CONFIG() };
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform));
    esp_zb_cfg_t nwk = ESP_ZB_ZC_CONFIG();
    esp_zb_init(&nwk);
    esp_zb_set_pan_id(ZB_PAN_ID);
    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(1UL << ZB_CHANNEL));
    esp_zb_ep_list_t *ep_list = NULL;
    ESP_ERROR_CHECK(create_endpoint(&ep_list));
    ESP_ERROR_CHECK(esp_zb_device_register(ep_list));
    esp_zb_core_action_handler_register(zb_action_handler);
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
    vTaskDelete(NULL);
}
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p = signal_struct->p_app_signal;
    esp_err_t st = signal_struct->esp_err_status;
    switch ((esp_zb_app_signal_type_t)*p) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP: bdb_start_cb(ESP_ZB_BDB_MODE_INITIALIZATION); break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (st == ESP_OK && esp_zb_bdb_is_factory_new()) bdb_start_cb(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        else if (st == ESP_OK) open_join_window();
        else {
            ESP_LOGW(TAG, "BU|ZB_COORD|START|FAIL|%s", esp_err_to_name(st));
            esp_zb_scheduler_alarm(bdb_start_cb, ESP_ZB_BDB_MODE_INITIALIZATION, ZB_COORD_RESET_DELAY_MS);
        }
        break;
    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (st == ESP_OK) open_join_window();
        else {
            ESP_LOGW(TAG, "BU|ZB_COORD|FORMATION|FAIL|%s", esp_err_to_name(st));
            esp_zb_scheduler_alarm(bdb_start_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION, ZB_COORD_RESET_DELAY_MS);
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
        esp_zb_zdo_signal_device_annce_params_t *a = esp_zb_app_signal_get_params(p);
        if (a != NULL) {
            char ieee[PROTO_IEEE_LEN];
            format_ieee(a->ieee_addr, ieee);
            cache_set(a->device_short_addr, ieee, true);
            push_event(ZB_EVT_JOIN, a->device_short_addr, ieee);
            ESP_LOGI(TAG, "BU|ZB_COORD|JOIN|SA=0x%04x|PASS", a->device_short_addr);
        }
        break;
    }
    case ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION: {
        esp_zb_zdo_signal_leave_indication_params_t *l = esp_zb_app_signal_get_params(p);
        if (l != NULL) handle_node_leave(l->short_addr, l->device_addr);
        break;
    }
    case ESP_ZB_ZDO_SIGNAL_DEVICE_UPDATE: {
        esp_zb_zdo_signal_device_update_params_t *u = esp_zb_app_signal_get_params(p);
        if (u != NULL && u->status == ESP_ZB_ZDO_STANDARD_DEV_LEFT && cache_get(u->short_addr, NULL)) handle_node_leave(u->short_addr, u->long_addr);
        break;
    }
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        taskENTER_CRITICAL(&s_mux); memset(s_nodes, 0, sizeof(s_nodes)); taskEXIT_CRITICAL(&s_mux);
        ESP_LOGW(TAG, "BU|ZB_COORD|LEAVE");
        esp_zb_scheduler_alarm(bdb_start_cb, ESP_ZB_BDB_MODE_NETWORK_FORMATION, ZB_COORD_RESET_DELAY_MS);
        break;
    default: break;
    }
}
QueueHandle_t zb_coord_evt_queue(void) { return s_queue; }
esp_err_t zb_coord_start(void)
{
    if (s_started) return ESP_ERR_INVALID_STATE;
    if ((s_queue = xQueueCreate(ZB_COORD_QUEUE_LEN, sizeof(zb_evt_t))) == NULL) return ESP_ERR_NO_MEM;
    if (xTaskCreate(zb_task, "zb_coord", ZB_COORD_TASK_STACK, NULL, ZB_COORD_TASK_PRIO, NULL) != pdPASS) { vQueueDelete(s_queue); s_queue = NULL; return ESP_ERR_NO_MEM; }
    s_started = true;
    return ESP_OK;
}

#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"

#include "config.h"
#include "node_zb_ep.h"
#include "version.h"

#define ESP_ZB_ZED_CONFIG()                                                     \
    {                                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                                   \
        .install_code_policy = ZB_INSTALL_CODE_POLICY,                          \
        .nwk_cfg.zed_cfg =                                                      \
            {                                                                   \
                .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,                    \
                .keep_alive = SPIKE_REPORT_MS,                                  \
            },                                                                  \
    }

#define ESP_ZB_DEFAULT_RADIO_CONFIG()                                           \
    {                                                                           \
        .radio_mode = ZB_RADIO_MODE_NATIVE,                                     \
    }

#define ESP_ZB_DEFAULT_HOST_CONFIG()                                            \
    {                                                                           \
        .host_connection_mode = ZB_HOST_CONNECTION_MODE_NONE,                   \
    }

static const char *TAG = "SNS";
static volatile bool s_joined;
static uint32_t s_seq;
static uint32_t s_send_fail_count;
static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    esp_err_t err = esp_zb_bdb_start_top_level_commissioning(mode_mask);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BDB start failed: %s", esp_err_to_name(err));
    }
}
static void reset_local_cb(uint8_t param)
{
    (void)param;
    esp_zb_bdb_reset_via_local_action();
}
static void request_rejoin(void)
{
    s_joined = false;
    esp_zb_scheduler_alarm(bdb_start_top_level_commissioning_cb,
                           ESP_ZB_BDB_MODE_NETWORK_STEERING, 0);
}
static void request_reset(void)
{
    s_joined = false;
    esp_zb_scheduler_alarm(reset_local_cb, 0, 0);
}
static esp_zb_zcl_status_t set_and_report(uint8_t endpoint, uint16_t cluster,
                                           uint16_t attr, void *value)
{
    esp_zb_zcl_status_t status = esp_zb_zcl_set_attribute_val(
        endpoint, cluster, ESP_ZB_ZCL_CLUSTER_SERVER_ROLE, attr, value, false);
    if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        return status;
    }
    esp_zb_zcl_report_attr_cmd_t cmd = {0};
    cmd.zcl_basic_cmd.dst_addr_u.addr_short = COORD_SHORT_ADDR;
    cmd.zcl_basic_cmd.dst_endpoint = COORD_EP;
    cmd.zcl_basic_cmd.src_endpoint = endpoint;
    cmd.address_mode = ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT;
    cmd.clusterID = cluster;
    cmd.direction = ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI;
    cmd.dis_default_resp = 1;
    cmd.manuf_code = ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC;
    cmd.attributeID = attr;
    return esp_zb_zcl_report_attr_cmd_req(&cmd) == ESP_OK
               ? ESP_ZB_ZCL_STATUS_SUCCESS
               : ESP_ZB_ZCL_STATUS_FAIL;
}
static esp_zb_zcl_status_t publish_fake_report(void)
{
    s_seq++;
    int16_t temp = (int16_t)lroundf(SPIKE_TEMP_C * ZCL_CENTI_SCALE);
    uint16_t hum = (uint16_t)lroundf(SPIKE_HUM_PCT * ZCL_CENTI_SCALE);
    float mq2 = SPIKE_MQ2_MV;
    float flame = SPIKE_FLAME_MV;
    float eco2 = SPIKE_ECO2_PPM;
    float tvoc = SPIKE_TVOC_PPB;
    float flags = SPIKE_FLAGS;
    const struct {
        uint8_t endpoint;
        uint16_t cluster;
        uint16_t attr;
        void *value;
    } reports[] = {
        {EP_TEMP, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT,
         ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &temp},
        {EP_HUM, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT,
         ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &hum},
        {EP_MQ2, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
         ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &mq2},
        {EP_FLAME, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
         ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &flame},
        {EP_ECO2, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
         ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &eco2},
        {EP_TVOC, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
         ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &tvoc},
        {EP_FLAGS, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
         ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &flags},
        {EP_FLAGS, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT,
         ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID, &s_seq},
    };
    for (size_t i = 0; i < sizeof(reports) / sizeof(reports[0]); i++) {
        esp_zb_zcl_status_t status = set_and_report(
            reports[i].endpoint, reports[i].cluster, reports[i].attr, reports[i].value);
        if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
            return status;
        }
    }
    return ESP_ZB_ZCL_STATUS_SUCCESS;
}
static void report_task(void *arg)
{
    (void)arg;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(SPIKE_REPORT_MS)); /* periodic */
        if (!s_joined) {
            continue;
        }
        if (!esp_zb_lock_acquire(pdMS_TO_TICKS(ZB_LOCK_TIMEOUT_MS))) {
            ESP_LOGW(TAG, "BU|ZB_SEND|FAIL|lock");
            continue;
        }
        esp_zb_zcl_status_t status = publish_fake_report();
        esp_zb_lock_release();
        if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
            ESP_LOGW(TAG, "BU|ZB_SEND|FAIL|status=0x%02x", status);
            if (++s_send_fail_count >= ZB_REJOIN_FAIL_THRESHOLD) {
                s_send_fail_count = 0;
                ESP_LOGW(TAG, "BU|ZB_NODE|REJOIN|REQUEST");
                request_reset();
            }
        } else {
            s_send_fail_count = 0;
        }
    }
}
static void zb_task(void *arg)
{
    (void)arg;
    esp_zb_platform_config_t platform_cfg = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG()};
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform_cfg));
    esp_zb_cfg_t nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&nwk_cfg);
    esp_zb_set_rx_on_when_idle(true);
    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(
        ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));
    esp_zb_ep_list_t *ep_list = NULL;
    ESP_ERROR_CHECK(node_zb_create_endpoints(&ep_list));
    ESP_ERROR_CHECK(esp_zb_device_register(ep_list));
    ESP_ERROR_CHECK(esp_zb_start(false));
    esp_zb_stack_main_loop();
    vTaskDelete(NULL);
}
void esp_zb_app_signal_handler(esp_zb_app_signal_t *signal_struct)
{
    uint32_t *p_sg_p = signal_struct->p_app_signal;
    esp_err_t err_status = signal_struct->esp_err_status;
    esp_zb_app_signal_type_t sig_type = *p_sg_p;
    switch (sig_type) {
    case ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP:
        bdb_start_top_level_commissioning_cb(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        if (err_status == ESP_OK && esp_zb_bdb_is_factory_new()) {
            ESP_LOGI(TAG, "BU|ZB_NODE|STEERING|t_ms=%u|PASS",
                     (unsigned)(esp_timer_get_time() / 1000));
            bdb_start_top_level_commissioning_cb(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else if (err_status == ESP_OK) {
            s_joined = true;
            ESP_LOGI(TAG, "BU|ZB_NODE|REJOINED|t_ms=%u|PASS",
                     (unsigned)(esp_timer_get_time() / 1000));
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        if (err_status == ESP_OK) {
            s_joined = true;
            ESP_LOGI(TAG,
                     "BU|ZB_NODE|JOINED|PAN=0x%04x|CH=%d|SA=0x%04x|t_ms=%u|PASS",
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(),
                     esp_zb_get_short_address(), (unsigned)(esp_timer_get_time() / 1000));
        } else {
            s_joined = false;
            ESP_LOGW(TAG, "BU|ZB_NODE|STEERING|FAIL|%s", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm(bdb_start_top_level_commissioning_cb,
                                   ESP_ZB_BDB_MODE_NETWORK_STEERING, ZB_RETRY_MIN_MS);
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGW(TAG, "BU|ZB_NODE|LEFT");
        request_rejoin();
        break;
    case ESP_ZB_NWK_SIGNAL_NO_ACTIVE_LINKS_LEFT:
        ESP_LOGW(TAG, "BU|ZB_NODE|NO_LINKS");
        request_rejoin();
        break;
    default:
        break;
    }
}
void app_main(void)
{
    ESP_LOGI(TAG, "firmware=%s", FW_NAME);
    ESP_LOGI(TAG, "version=%s", FW_VERSION);
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());
    ESP_ERROR_CHECK(nvs_flash_init());
    xTaskCreate(zb_task, "zb_node", ZB_TASK_STACK, NULL, ZB_TASK_PRIO, NULL);
    xTaskCreate(report_task, "zb_report", ZB_TASK_STACK, NULL, ZB_TASK_PRIO - 1, NULL);
}

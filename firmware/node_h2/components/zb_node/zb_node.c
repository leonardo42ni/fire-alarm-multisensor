#include "zb_node.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"

#include "shared_config.h"
#include "zb_node_config.h"
#include "zb_node_internal.h"

#define ZCL_TEMP_INVALID ((int16_t)0x8000)
#define ZCL_HUM_INVALID ((uint16_t)0xFFFF)

static const char *TAG = "ZBN";
static uint32_t s_tx_fail;
static bool s_started;

static int16_t scale_temp_centi(float celsius)
{
    if (!isfinite(celsius)) {
        return ZCL_TEMP_INVALID;
    }
    float scaled = celsius * ZCL_CENTI_SCALE;
    if (!isfinite(scaled)) {
        return ZCL_TEMP_INVALID;
    }
    if (scaled < (float)ESP_ZB_ZCL_TEMP_MEASUREMENT_MIN_MEASURED_VALUE_MINIMUM) {
        scaled = (float)ESP_ZB_ZCL_TEMP_MEASUREMENT_MIN_MEASURED_VALUE_MINIMUM;
    }
    if (scaled > (float)ESP_ZB_ZCL_TEMP_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM) {
        scaled = (float)ESP_ZB_ZCL_TEMP_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM;
    }
    return (int16_t)lroundf(scaled);
}

static uint16_t scale_hum_centi(float percent)
{
    if (!isfinite(percent)) {
        return ZCL_HUM_INVALID;
    }
    float scaled = percent * ZCL_CENTI_SCALE;
    if (!isfinite(scaled)) {
        return ZCL_HUM_INVALID;
    }
    if (scaled < 0.0f) {
        scaled = 0.0f;
    }
    if (scaled > (float)ESP_ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM) {
        scaled = (float)ESP_ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM;
    }
    return (uint16_t)lroundf(scaled);
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

static esp_err_t tx_failed(esp_zb_zcl_status_t status)
{
    s_tx_fail++;
    ESP_LOGW(TAG, "BU|ZB_SEND|FAIL|status=0x%02x", status);
    if (s_tx_fail >= ZB_REJOIN_FAIL_THRESHOLD) {
        s_tx_fail = 0;
        ESP_LOGW(TAG, "BU|ZB_NODE|REJOIN|REQUEST");
        zb_node_request_reset_app();
    }
    return ESP_FAIL;
}

esp_err_t zb_node_publish(const node_snapshot_t *snap)
{
    if (snap == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    if (!zb_node_joined()) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!esp_zb_lock_acquire(pdMS_TO_TICKS(ZB_LOCK_TIMEOUT_MS))) {
        ESP_LOGW(TAG, "BU|ZB_SEND|FAIL|lock");
        return ESP_ERR_TIMEOUT;
    }
    int16_t temp = scale_temp_centi(snap->temp);
    uint16_t hum = scale_hum_centi(snap->hum);
    float mq2 = snap->mq2_mv;
    float flame = snap->flame_mv;
    float eco2 = (float)snap->eco2;
    float tvoc = (float)snap->tvoc;
    uint16_t flags_u16 = snap->flags;
    if (snap->flame_do) {
        flags_u16 |= ZB_NODE_FLAG_FLAME_DO;
    }
    if (snap->prealert) {
        flags_u16 |= ZB_NODE_FLAG_PREALERT;
    }
    float flags = (float)flags_u16;
    uint32_t seq = snap->seq;
    const struct {
        uint8_t endpoint;
        uint16_t cluster;
        uint16_t attr;
        void *value;
    } reports[] = {
        {EP_TEMP, ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT, ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID, &temp},
        {EP_HUM, ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT, ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID, &hum},
        {EP_MQ2, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &mq2},
        {EP_FLAME, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &flame},
        {EP_ECO2, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &eco2},
        {EP_TVOC, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &tvoc},
        {EP_FLAGS, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID, &flags},
        {EP_FLAGS, ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT, ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID, &seq},
    };
    esp_zb_zcl_status_t status = ESP_ZB_ZCL_STATUS_SUCCESS;
    for (size_t i = 0; i < sizeof(reports) / sizeof(reports[0]); i++) {
        status = set_and_report(reports[i].endpoint, reports[i].cluster,
                                reports[i].attr, reports[i].value);
        if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
            break;
        }
    }
    esp_zb_lock_release();
    if (status != ESP_ZB_ZCL_STATUS_SUCCESS) {
        return tx_failed(status);
    }
    s_tx_fail = 0;
    return ESP_OK;
}

esp_err_t zb_node_start(void)
{
    if (s_started) {
        return ESP_ERR_INVALID_STATE;
    }
    BaseType_t ok = xTaskCreate(zb_node_task, "zb_node", ZB_TASK_STACK, NULL,
                                ZB_TASK_PRIO, NULL);
    if (ok != pdPASS) {
        return ESP_ERR_NO_MEM;
    }
    s_started = true;
    return ESP_OK;
}

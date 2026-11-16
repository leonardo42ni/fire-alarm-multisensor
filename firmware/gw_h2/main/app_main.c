#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"
#include "nvs_flash.h"
#include "zcl/esp_zigbee_zcl_analog_input.h"

#include "config.h"
#include "version.h"

#define ESP_ZB_ZC_CONFIG()                                                      \
    {                                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_COORDINATOR,                          \
        .install_code_policy = ZB_INSTALL_CODE_POLICY,                          \
        .nwk_cfg.zczr_cfg =                                                     \
            {                                                                   \
                .max_children = NC_MAX,                                         \
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

static const char *TAG = "ZB";
static volatile bool s_network_formed;

static void bdb_start_top_level_commissioning_cb(uint8_t mode_mask)
{
    esp_err_t err = esp_zb_bdb_start_top_level_commissioning(mode_mask);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BDB start failed: %s", esp_err_to_name(err));
    }
}

static void open_join_window(void)
{
    esp_err_t err = esp_zb_bdb_open_network(JOIN_WINDOW_S);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "BU|ZB_COORD|PERMIT|FAIL|%s", esp_err_to_name(err));
    }
}

static float report_attr_value(uint16_t cluster, const esp_zb_zcl_attribute_t *attribute)
{
    if (attribute->data.value == NULL) {
        return 0.0f;
    }

    float value;
    switch (attribute->data.type) {
    case ESP_ZB_ZCL_ATTR_TYPE_S16:
        value = (float)*(int16_t *)attribute->data.value;
        break;
    case ESP_ZB_ZCL_ATTR_TYPE_U16:
        value = (float)*(uint16_t *)attribute->data.value;
        break;
    case ESP_ZB_ZCL_ATTR_TYPE_U32:
        value = (float)*(uint32_t *)attribute->data.value;
        break;
    case ESP_ZB_ZCL_ATTR_TYPE_SINGLE:
        value = *(float *)attribute->data.value;
        break;
    default:
        return 0.0f;
    }

    if (cluster == ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT ||
        cluster == ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT) {
        value /= ZCL_CENTI_SCALE;
    }
    return value;
}

static esp_err_t zb_action_handler(esp_zb_core_action_callback_id_t callback_id,
                                   const void *message)
{
    if (callback_id == ESP_ZB_CORE_REPORT_ATTR_CB_ID) {
        const esp_zb_zcl_report_attr_message_t *report = message;
        if (report != NULL && report->status == ESP_ZB_ZCL_STATUS_SUCCESS) {
            if (report->cluster == ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT &&
                report->attribute.id == ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID &&
                report->attribute.data.value != NULL) {
                ESP_LOGI(TAG, "BU|ZB_SEQ|seq=%u|PASS",
                         *(uint32_t *)report->attribute.data.value);
            }
            ESP_LOGI(TAG, "BU|ZB_RECV|ep=%d|cl=0x%04x|attr=0x%04x|val=%.2f|PASS",
                     report->src_endpoint, report->cluster, report->attribute.id,
                     report_attr_value(report->cluster, &report->attribute));
        }
    }
    return ESP_OK;
}

static esp_err_t create_coordinator_endpoint(esp_zb_ep_list_t **out_ep_list)
{
    esp_zb_ep_list_t *ep_list = esp_zb_ep_list_create();
    esp_zb_cluster_list_t *cluster_list = esp_zb_zcl_cluster_list_create();
    if (ep_list == NULL || cluster_list == NULL) {
        return ESP_ERR_NO_MEM;
    }

    esp_zb_basic_cluster_cfg_t basic_cfg = {
        .zcl_version = ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE,
        .power_source = ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE,
    };
    esp_err_t err = esp_zb_cluster_list_add_basic_cluster(
        cluster_list, esp_zb_basic_cluster_create(&basic_cfg),
        ESP_ZB_ZCL_CLUSTER_SERVER_ROLE);
    if (err != ESP_OK) {
        return err;
    }

    esp_zb_endpoint_config_t endpoint_cfg = {
        .endpoint = COORD_EP,
        .app_profile_id = ESP_ZB_AF_HA_PROFILE_ID,
        .app_device_id = ESP_ZB_HA_HOME_GATEWAY_DEVICE_ID,
        .app_device_version = 0,
    };
    err = esp_zb_ep_list_add_gateway_ep(ep_list, cluster_list, endpoint_cfg);
    if (err != ESP_OK) {
        return err;
    }

    *out_ep_list = ep_list;
    return ESP_OK;
}

static void zb_task(void *arg)
{
    (void)arg;
    esp_zb_platform_config_t platform_cfg = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG(),
    };
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform_cfg));

    esp_zb_cfg_t nwk_cfg = ESP_ZB_ZC_CONFIG();
    esp_zb_init(&nwk_cfg);
    esp_zb_set_pan_id(ZB_PAN_ID);
    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(1UL << ZB_CHANNEL));

    esp_zb_ep_list_t *ep_list = NULL;
    ESP_ERROR_CHECK(create_coordinator_endpoint(&ep_list));
    ESP_ERROR_CHECK(esp_zb_device_register(ep_list));
    esp_zb_core_action_handler_register(zb_action_handler);
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
            bdb_start_top_level_commissioning_cb(ESP_ZB_BDB_MODE_NETWORK_FORMATION);
        } else if (err_status == ESP_OK) {
            s_network_formed = true;
            open_join_window();
        }
        break;
    case ESP_ZB_BDB_SIGNAL_FORMATION:
        if (err_status == ESP_OK) {
            s_network_formed = true;
            ESP_LOGI(TAG, "BU|ZB_COORD|FORMED|PAN=0x%04x|CH=%d|SA=0x%04x|PASS",
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(),
                     esp_zb_get_short_address());
            open_join_window();
        } else {
            ESP_LOGW(TAG, "BU|ZB_COORD|FORMATION|FAIL|%s", esp_err_to_name(err_status));
            esp_zb_scheduler_alarm(bdb_start_top_level_commissioning_cb,
                                   ESP_ZB_BDB_MODE_NETWORK_FORMATION,
                                   ZB_COORD_RESET_DELAY_MS);
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE: {
        esp_zb_zdo_signal_device_annce_params_t *params =
            esp_zb_app_signal_get_params(p_sg_p);
        ESP_LOGI(TAG, "BU|ZB_COORD|JOIN|SA=0x%04x|PASS", params->device_short_addr);
        break;
    }
    case ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS: {
        uint8_t *duration = esp_zb_app_signal_get_params(p_sg_p);
        if (err_status == ESP_OK && duration != NULL) {
            ESP_LOGI(TAG, "BU|ZB_COORD|PERMIT|%u", *duration);
        }
        break;
    }
    default:
        if (s_network_formed) {
            ESP_LOGD(TAG, "signal=%s(0x%x) status=%s", esp_zb_zdo_signal_to_string(sig_type),
                     sig_type, esp_err_to_name(err_status));
        }
        break;
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "firmware=%s", FW_NAME);
    ESP_LOGI(TAG, "version=%s", FW_VERSION);
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());

    ESP_ERROR_CHECK(nvs_flash_init());
    xTaskCreate(zb_task, "zb_coord", ZB_TASK_STACK, NULL, ZB_TASK_PRIO, NULL);
}

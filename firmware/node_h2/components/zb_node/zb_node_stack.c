#include "zb_node_internal.h"
#include "zb_node.h"

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ha/esp_zigbee_ha_standard.h"

#include "shared_config.h"
#include "zb_node_config.h"

#define ESP_ZB_ZED_CONFIG()                                                     \
    {                                                                           \
        .esp_zb_role = ESP_ZB_DEVICE_TYPE_ED,                                   \
        .install_code_policy = ZB_INSTALL_CODE_POLICY,                          \
        .nwk_cfg.zed_cfg =                                                      \
            {                                                                   \
                .ed_timeout = ESP_ZB_ED_AGING_TIMEOUT_64MIN,                    \
                .keep_alive = ZB_KEEP_ALIVE_MS,                                 \
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

static const char *TAG = "ZBN";
static atomic_bool s_joined;
static atomic_bool s_reset_pending;
static uint32_t s_retry_ms = ZB_RETRY_MIN_MS;

static void bdb_start_cb(uint8_t mode_mask)
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
    zb_node_set_joined(false);
    esp_zb_scheduler_alarm(bdb_start_cb, ESP_ZB_BDB_MODE_NETWORK_STEERING, 0);
}

static uint32_t backoff_next(void)
{
    uint32_t delay = s_retry_ms;
    if (s_retry_ms >= ZB_RETRY_MAX_MS / 2u) {
        s_retry_ms = ZB_RETRY_MAX_MS;
    } else {
        s_retry_ms *= 2u;
    }
    return delay;
}

void zb_node_set_joined(bool joined)
{
    atomic_store_explicit(&s_joined, joined, memory_order_release);
    if (joined) {
        s_retry_ms = ZB_RETRY_MIN_MS;
    }
}

bool zb_node_joined(void)
{
    return atomic_load_explicit(&s_joined, memory_order_acquire);
}

static void clear_reset_pending(void)
{
    atomic_store_explicit(&s_reset_pending, false, memory_order_release);
}

static void request_reset_locked(void)
{
    if (atomic_exchange_explicit(&s_reset_pending, true, memory_order_acq_rel)) {
        return;
    }
    zb_node_set_joined(false);
    esp_zb_scheduler_alarm(reset_local_cb, 0, 0);
}

void zb_node_request_reset_app(void)
{
    if (!esp_zb_lock_acquire(pdMS_TO_TICKS(ZB_LOCK_TIMEOUT_MS))) {
        ESP_LOGW(TAG, "BU|ZB_NODE|RESET|FAIL|lock");
        return;
    }
    request_reset_locked();
    esp_zb_lock_release();
}

void zb_node_task(void *arg)
{
    (void)arg;
    esp_zb_platform_config_t platform_cfg = {
        .radio_config = ESP_ZB_DEFAULT_RADIO_CONFIG(),
        .host_config = ESP_ZB_DEFAULT_HOST_CONFIG()};
    ESP_ERROR_CHECK(esp_zb_platform_config(&platform_cfg));
    esp_zb_cfg_t nwk_cfg = ESP_ZB_ZED_CONFIG();
    esp_zb_init(&nwk_cfg);
    esp_zb_set_rx_on_when_idle(true);
    esp_zb_ep_list_t *ep_list = NULL;
    ESP_ERROR_CHECK(zb_node_create_endpoints(&ep_list));
    ESP_ERROR_CHECK(esp_zb_device_register(ep_list));
    ESP_ERROR_CHECK(esp_zb_set_primary_network_channel_set(
        ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK));
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
        bdb_start_cb(ESP_ZB_BDB_MODE_INITIALIZATION);
        break;
    case ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START:
    case ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT:
        clear_reset_pending();
        if (err_status == ESP_OK && esp_zb_bdb_is_factory_new()) {
            ESP_LOGI(TAG, "BU|ZB_NODE|STEERING|t_ms=%u|PASS",
                     (unsigned)(esp_timer_get_time() / 1000));
            bdb_start_cb(ESP_ZB_BDB_MODE_NETWORK_STEERING);
        } else if (err_status == ESP_OK) {
            zb_node_set_joined(true);
            ESP_LOGI(TAG, "BU|ZB_NODE|REJOINED|t_ms=%u|PASS",
                     (unsigned)(esp_timer_get_time() / 1000));
        } else {
            uint32_t retry_ms = backoff_next();
            zb_node_set_joined(false);
            ESP_LOGW(TAG, "BU|ZB_NODE|START|FAIL|%s|retry_ms=%u",
                     esp_err_to_name(err_status), (unsigned)retry_ms);
            esp_zb_scheduler_alarm(bdb_start_cb,
                                   ESP_ZB_BDB_MODE_INITIALIZATION, retry_ms);
        }
        break;
    case ESP_ZB_BDB_SIGNAL_STEERING:
        clear_reset_pending();
        if (err_status == ESP_OK) {
            zb_node_set_joined(true);
            ESP_LOGI(TAG,
                     "BU|ZB_NODE|JOINED|PAN=0x%04x|CH=%d|SA=0x%04x|t_ms=%u|PASS",
                     esp_zb_get_pan_id(), esp_zb_get_current_channel(),
                     esp_zb_get_short_address(),
                     (unsigned)(esp_timer_get_time() / 1000));
        } else {
            uint32_t retry_ms = backoff_next();
            zb_node_set_joined(false);
            ESP_LOGW(TAG, "BU|ZB_NODE|STEERING|FAIL|%s|retry_ms=%u",
                     esp_err_to_name(err_status), (unsigned)retry_ms);
            esp_zb_scheduler_alarm(bdb_start_cb,
                                   ESP_ZB_BDB_MODE_NETWORK_STEERING, retry_ms);
        }
        break;
    case ESP_ZB_ZDO_SIGNAL_LEAVE:
        ESP_LOGW(TAG, "BU|ZB_NODE|LEFT");
        clear_reset_pending();
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

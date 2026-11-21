#include "net_mqtt.h"

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_crt_bundle.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "mqtt_client.h"
#include "nvs_flash.h"

static const char *TAG = "MQT";
static esp_mqtt_client_handle_t s_client;
static bool s_started;
static bool s_sntp_started;
static bool s_mqtt_started;
static atomic_bool s_connected = ATOMIC_VAR_INIT(false);

#define APP_JSON(expr)                                                            \
    do {                                                                          \
        int _n = (expr);                                                          \
        if (_n < 0 || (size_t)_n >= sizeof(payload) - off) return ESP_ERR_NO_MEM; \
        off += (size_t)_n;                                                        \
    } while (0)

static esp_err_t nvs_init_safe(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "BU|NET_MQTT|NVS|ERASE|%s", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err == ESP_OK) {
            err = nvs_flash_init();
        }
    }
    return err;
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "BU|NET_MQTT|WIFI|START");
        (void)esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        atomic_store(&s_connected, false);
        ESP_LOGW(TAG, "BU|NET_MQTT|WIFI|DISCONN|reconnecting");
        (void)esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *ev = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "BU|NET_MQTT|WIFI|GOT_IP|" IPSTR, IP2STR(&ev->ip_info.ip));
        if (!s_sntp_started) {
            esp_sntp_config_t scfg = ESP_NETIF_SNTP_DEFAULT_CONFIG(NET_MQTT_SNTP_SERVER);
            if (esp_netif_sntp_init(&scfg) == ESP_OK) {
                s_sntp_started = true;
            }
        }
        if (!s_mqtt_started && s_client != NULL) {
            if (esp_mqtt_client_start(s_client) == ESP_OK) {
                s_mqtt_started = true;
            }
        }
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg;
    (void)base;
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)data;
    switch ((esp_mqtt_event_id_t)id) {
    case MQTT_EVENT_CONNECTED:
        atomic_store(&s_connected, true);
        ESP_LOGI(TAG, "BU|NET_MQTT|CONN|ONLINE");
        (void)esp_mqtt_client_enqueue(s_client, NET_MQTT_TOPIC_STATUS, "online", 0, 1, 1, true);
        break;
    case MQTT_EVENT_DISCONNECTED:
        atomic_store(&s_connected, false);
        ESP_LOGW(TAG, "BU|NET_MQTT|CONN|OFFLINE");
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGW(TAG, "BU|NET_MQTT|ERR|type=%d",
                 (ev != NULL && ev->error_handle != NULL) ? (int)ev->error_handle->error_type : -1);
        break;
    default:
        break;
    }
}

esp_err_t net_mqtt_start(const net_mqtt_cfg_t *cfg)
{
    if (s_started) {
        return ESP_ERR_INVALID_STATE;
    }
    if (cfg == NULL || cfg->wifi_ssid == NULL || cfg->mqtt_host == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t err = nvs_init_safe();
    if (err != ESP_OK) return err;
    err = esp_netif_init();
    if (err != ESP_OK) return err;
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) return err;
    if (esp_netif_create_default_wifi_sta() == NULL) return ESP_FAIL;

    const esp_mqtt_client_config_t mcfg = {
        .broker.address.hostname = cfg->mqtt_host,
        .broker.address.port = cfg->mqtt_port,
        .broker.address.transport = MQTT_TRANSPORT_OVER_SSL,
        .broker.verification.crt_bundle_attach = esp_crt_bundle_attach,
        .credentials.username = cfg->mqtt_user,
        .credentials.client_id = cfg->mqtt_client_id,
        .credentials.authentication.password = cfg->mqtt_pass,
        .session.keepalive = NET_MQTT_KEEPALIVE_S,
        .session.last_will = {
            .topic = NET_MQTT_TOPIC_STATUS,
            .msg = "offline",
            .msg_len = 7,
            .qos = 1,
            .retain = 1,
        },
    };
    s_client = esp_mqtt_client_init(&mcfg);
    if (s_client == NULL) return ESP_FAIL;
    err = esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    if (err != ESP_OK) return err;

    wifi_init_config_t winit = WIFI_INIT_CONFIG_DEFAULT();
    err = esp_wifi_init(&winit);
    if (err != ESP_OK) return err;
    err = esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) return err;
    err = esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);
    if (err != ESP_OK) return err;

    wifi_config_t wcfg = {0};
    wcfg.sta.scan_method = WIFI_ALL_CHANNEL_SCAN;
    wcfg.sta.sort_method = WIFI_CONNECT_AP_BY_SIGNAL;
    snprintf((char *)wcfg.sta.ssid, sizeof(wcfg.sta.ssid), "%s", cfg->wifi_ssid);
    snprintf((char *)wcfg.sta.password, sizeof(wcfg.sta.password), "%s",
             (cfg->wifi_pass != NULL) ? cfg->wifi_pass : "");

    err = esp_wifi_set_mode(WIFI_MODE_STA);
    if (err == ESP_OK) err = esp_wifi_set_config(WIFI_IF_STA, &wcfg);
    if (err == ESP_OK) err = esp_wifi_start();
    if (err != ESP_OK) return err;

    s_started = true;
    ESP_LOGI(TAG, "BU|NET_MQTT|START|port=%u", (unsigned)cfg->mqtt_port);
    return ESP_OK;
}

bool net_mqtt_connected(void) { return atomic_load(&s_connected); }

esp_err_t net_mqtt_publish_sensor(const rpt_t *r, float risk, int state)
{
    if (!atomic_load(&s_connected) || s_client == NULL || r == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    const char *node = (r->ieee[0] != '\0') ? r->ieee : "unknown";
    char topic[NET_MQTT_TOPIC_MAX];
    int tn = snprintf(topic, sizeof(topic), "%s%s", NET_MQTT_TOPIC_SENSOR_PREFIX, node);
    if (tn <= 0 || tn >= (int)sizeof(topic)) return ESP_ERR_INVALID_ARG;

    char payload[NET_MQTT_PAYLOAD_MAX];
    size_t off = 0;
    APP_JSON(snprintf(payload + off, sizeof(payload) - off, "{\"node_id\":\"%s\",\"seq\":%lu",
                      node, (unsigned long)r->seq));
    if ((r->valid & RPT_HAS_TEMP) != 0u && isfinite(r->temp)) {
        APP_JSON(snprintf(payload + off, sizeof(payload) - off, ",\"temp\":%.2f", (double)r->temp));
    }
    if ((r->valid & RPT_HAS_HUM) != 0u && isfinite(r->hum)) {
        APP_JSON(snprintf(payload + off, sizeof(payload) - off, ",\"hum\":%.2f", (double)r->hum));
    }
    if ((r->valid & RPT_HAS_ECO2) != 0u && isfinite(r->eco2)) {
        APP_JSON(snprintf(payload + off, sizeof(payload) - off, ",\"eco2\":%.0f", (double)r->eco2));
    }
    if ((r->valid & RPT_HAS_TVOC) != 0u && isfinite(r->tvoc)) {
        APP_JSON(snprintf(payload + off, sizeof(payload) - off, ",\"tvoc\":%.0f", (double)r->tvoc));
    }
    if ((r->valid & RPT_HAS_MQ2) != 0u && isfinite(r->mq2_mv)) {
        APP_JSON(snprintf(payload + off, sizeof(payload) - off, ",\"mq2_mv\":%.1f", (double)r->mq2_mv));
    }
    if ((r->valid & RPT_HAS_FLAME) != 0u && isfinite(r->flame_mv)) {
        APP_JSON(snprintf(payload + off, sizeof(payload) - off, ",\"flame_mv\":%.1f", (double)r->flame_mv));
    }

    unsigned flags = ((r->valid & RPT_HAS_FLAGS) != 0u) ? (unsigned)r->flags : 0u;
    float safe_risk = isfinite(risk) ? risk : 0.0f;
    time_t now_s = time(NULL);
    long ts = (now_s >= (time_t)NET_MQTT_MIN_EPOCH) ? (long)now_s : 0L;
    APP_JSON(snprintf(payload + off, sizeof(payload) - off,
                      ",\"flags\":%u,\"risk\":%.1f,\"state\":%d,\"ts\":%ld}",
                      flags, (double)safe_risk, state, ts));

    int msg_id = esp_mqtt_client_enqueue(s_client, topic, payload, (int)off, 0, 0, true);
    if (msg_id < 0) return ESP_FAIL;
    ESP_LOGI(TAG, "BU|NET_MQTT|PUB|topic=%s|seq=%lu", topic, (unsigned long)r->seq);
    return ESP_OK;
}

esp_err_t net_mqtt_publish_alert(const char *node_id, float risk, int state)
{
    if (!atomic_load(&s_connected) || s_client == NULL || node_id == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    char topic[NET_MQTT_TOPIC_MAX];
    int tn = snprintf(topic, sizeof(topic), "%s%s", NET_MQTT_TOPIC_ALERT_PREFIX, node_id);
    if (tn <= 0 || tn >= (int)sizeof(topic)) return ESP_ERR_INVALID_ARG;

    char payload[NET_MQTT_PAYLOAD_MAX];
    float safe_risk = isfinite(risk) ? risk : 0.0f;
    time_t now_s = time(NULL);
    long ts = (now_s >= (time_t)NET_MQTT_MIN_EPOCH) ? (long)now_s : 0L;
    int pn = snprintf(payload, sizeof(payload),
                      "{\"node_id\":\"%s\",\"risk\":%.1f,\"state\":%d,\"ts\":%ld}",
                      node_id, (double)safe_risk, state, ts);
    if (pn <= 0 || pn >= (int)sizeof(payload)) return ESP_ERR_NO_MEM;

    int msg_id = esp_mqtt_client_enqueue(s_client, topic, payload, pn, 1, 0, true);
    return (msg_id < 0) ? ESP_FAIL : ESP_OK;
}

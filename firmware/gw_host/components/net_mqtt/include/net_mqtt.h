#ifndef NET_MQTT_H
#define NET_MQTT_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "proto_ndjson.h"

#define NET_MQTT_SNTP_SERVER "pool.ntp.org"
#define NET_MQTT_TOPIC_SENSOR_PREFIX "sensor/"
#define NET_MQTT_TOPIC_ALERT_PREFIX "alert/"
#define NET_MQTT_TOPIC_STATUS "status/gateway"
#define NET_MQTT_KEEPALIVE_S 30
#define NET_MQTT_TOPIC_MAX 48
#define NET_MQTT_PAYLOAD_MAX 320
#define NET_MQTT_MIN_EPOCH 1700000000L

typedef struct {
    const char *wifi_ssid;
    const char *wifi_pass;
    const char *mqtt_host;
    uint16_t mqtt_port;
    const char *mqtt_user;
    const char *mqtt_pass;
    const char *mqtt_client_id;
} net_mqtt_cfg_t;

esp_err_t net_mqtt_start(const net_mqtt_cfg_t *cfg);
bool net_mqtt_connected(void);
esp_err_t net_mqtt_publish_sensor(const rpt_t *r, float risk, int state);
esp_err_t net_mqtt_publish_alert(const char *node_id, float risk, int state);

#endif /* NET_MQTT_H */

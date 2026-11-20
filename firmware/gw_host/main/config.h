#ifndef GW_HOST_CONFIG_H
#define GW_HOST_CONFIG_H

#include <stddef.h>

#include "shared_config.h"

#if !__has_include("secrets.h")
#error "Thieu main/secrets.h: sao chep tu secrets.example.h va dien thong tin"
#endif
#include "secrets.h"

#ifndef WIFI_SSID
#error "Thieu WIFI_SSID trong secrets.h"
#endif
#ifndef WIFI_PASS
#error "Thieu WIFI_PASS trong secrets.h"
#endif
#ifndef MQTT_HOST
#error "Thieu MQTT_HOST trong secrets.h"
#endif
#ifndef MQTT_PORT
#error "Thieu MQTT_PORT trong secrets.h"
#endif
#ifndef MQTT_USER
#error "Thieu MQTT_USER trong secrets.h"
#endif
#ifndef MQTT_PASS
#error "Thieu MQTT_PASS trong secrets.h"
#endif
#ifndef MQTT_CLIENT_ID
#error "Thieu MQTT_CLIENT_ID trong secrets.h"
#endif

#define FW_NAME "gw_host"

#define UART_TX_GPIO 17
#define UART_RX_GPIO 16
#define UART_READ_CHUNK 128
#define UART_READ_TIMEOUT_MS 100

#define RGB_R_GPIO 25
#define RGB_G_GPIO 26
#define RGB_B_GPIO 27
#define RGB_LEDC_FREQ_HZ 5000u
#define RGB_LEDC_RES_BITS 8
#define RGB_YELLOW_R_DUTY 255
#define RGB_YELLOW_G_DUTY 160

#define BUZZER_GPIO 4
#define BUZZER_PASSIVE 0
#define BUZZER_PASSIVE_FREQ_HZ 2700u

#define BUTTON_GPIO 32
#define INPUT_TASK_MS 10u
#define ALARM_TASK_MS 50u
#define BTN_INTEG_MAX 3
#define BTN_SHORT_MIN_MS 50u
#define BTN_SHORT_MAX_MS 999u
#define BTN_RESET_MIN_MS 3000u
#define BTN_RESET_MAX_MS 4999u
#define BTN_JOIN_MIN_MS 5000u

#define PWR_ADC_GPIO 35
#define PWR_SAMPLE_MS 1000u
#define PWR_ADC_SAMPLES 16
#define PS_DIV_NUM 25
#define PS_DIV_DEN 15
#define PS_ON_MV 2300
#define PS_OFF_MV 1500
#define PS_DEBOUNCE_N 3

#define STATUS_PERIOD_MS 30000u
#define MUTE_DEFAULT_S 300u
#define CUE_MAX_MS 600u
#define SELFTEST_MS 1200u
#define SELFTEST_STEP_MS 700u
#define TEST_ALARM_MS 5000u

#define NR_MAX 4
#define NODE_NAME_LEN 16
#define Q_RPT_LEN 8
#define Q_BTN_LEN 4

#define MQTT_TOPIC_SENSOR_PREFIX "sensor/"
#define MQTT_TOPIC_ALERT_PREFIX "alert/"
#define MQTT_TOPIC_STATUS "status/gateway"
#define MQTT_TOPIC_CMD "cmd/gateway"
#define MQTT_KEEPALIVE_S 30
#define MQTT_PUB_RETRY_MS 200u
#define MQTT_OFFLINE_WAIT_MS 500u
#define PAYLOAD_MAX 320
#define CMD_MAX_LEN 128
#define CMD_MIN_INTERVAL_MS 1000u

typedef struct {
    const char *ieee;
    const char *name;
} node_map_t;

extern const node_map_t NODES_MAP[];
extern const size_t NODES_MAP_LEN;

#endif

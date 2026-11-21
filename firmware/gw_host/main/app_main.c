#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "config.h"
#include "net_mqtt.h"
#include "proto_ndjson.h"
#include "uart_link.h"
#include "version.h"

static const char *TAG = "HST";

static void safe_boot_outputs(void)
{
    gpio_set_level(BUZZER_GPIO, 0);
    gpio_set_level(RGB_R_GPIO, 0);
    gpio_set_level(RGB_G_GPIO, 0);
    gpio_set_level(RGB_B_GPIO, 0);

    const gpio_config_t io = {
        .pin_bit_mask = SAFE_BOOT_GPIO_MASK,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io);
    gpio_set_level(BUZZER_GPIO, 0);
    gpio_set_level(RGB_R_GPIO, 0);
    gpio_set_level(RGB_G_GPIO, 0);
    gpio_set_level(RGB_B_GPIO, 0);
}

void app_main(void)
{
    safe_boot_outputs();

    ESP_LOGI(TAG, "firmware=%s", FW_NAME);
    ESP_LOGI(TAG, "version=%s", FW_VERSION);
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());

    ESP_ERROR_CHECK(uart_link_start());

    const net_mqtt_cfg_t mcfg = {
        .wifi_ssid = WIFI_SSID,
        .wifi_pass = WIFI_PASS,
        .mqtt_host = MQTT_HOST,
        .mqtt_port = (uint16_t)MQTT_PORT,
        .mqtt_user = MQTT_USER,
        .mqtt_pass = MQTT_PASS,
        .mqtt_client_id = MQTT_CLIENT_ID,
    };
    esp_err_t net_err = net_mqtt_start(&mcfg);
    if (net_err != ESP_OK) {
        ESP_LOGW(TAG, "BU|HOST|NET_MQTT|FAIL|%s", esp_err_to_name(net_err));
    }

    QueueHandle_t q = uart_link_rpt_queue();
    int ping_left = 2;
    rpt_t rpt;

    for (;;) {
        if (xQueueReceive(q, &rpt, pdMS_TO_TICKS(RPT_POLL_MS)) == pdTRUE) {
            uint32_t t_ms = (uint32_t)(esp_timer_get_time() / (int64_t)UART_LINK_MS_PER_S);
            uart_link_print_csv(&rpt, t_ms);
            (void)net_mqtt_publish_sensor(&rpt, 0.0f, 0);
        } else if (ping_left > 0) {
            ping_left--;
            esp_err_t err = uart_link_send_cmd(OP_PING, 0);
            ESP_LOGI(TAG, "BU|HOST|PING|%s", esp_err_to_name(err));
        }
    }
}

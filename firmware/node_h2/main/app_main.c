#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "config.h"
#include "version.h"
#include "zb_node.h"

static const char *TAG = "SNS";

static esp_err_t nvs_init_safe(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS init failed: %s, erasing", esp_err_to_name(err));
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            return err;
        }
        err = nvs_flash_init();
    }
    return err;
}

static void report_task(void *arg)
{
    (void)arg;
    ESP_ERROR_CHECK(esp_task_wdt_add(NULL));
    TickType_t last = xTaskGetTickCount();
    uint32_t seq = 0;
    for (;;) {
        vTaskDelayUntil(&last, pdMS_TO_TICKS(SPIKE_REPORT_MS)); /* periodic */
        esp_err_t wdt_err = esp_task_wdt_reset();
        if (wdt_err != ESP_OK) {
            ESP_LOGW(TAG, "TWDT reset failed: %s", esp_err_to_name(wdt_err));
        }
        if (!zb_node_joined()) {
            continue;
        }
        node_snapshot_t snap = {
            .t_ms = (uint32_t)(esp_timer_get_time() / 1000),
            .temp = SPIKE_TEMP_C,
            .hum = SPIKE_HUM_PCT,
            .eco2 = (uint16_t)SPIKE_ECO2_PPM,
            .tvoc = (uint16_t)SPIKE_TVOC_PPB,
            .mq2_mv = SPIKE_MQ2_MV,
            .flame_mv = SPIKE_FLAME_MV,
            .flame_do = false,
            .prealert = false,
            .flags = (uint16_t)SPIKE_FLAGS,
            .seq = ++seq,
        };
        esp_err_t err = zb_node_publish(&snap);
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_LOGW(TAG, "BU|ZB_SEND|FAIL|%s", esp_err_to_name(err));
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "firmware=%s", FW_NAME);
    ESP_LOGI(TAG, "version=%s", FW_VERSION);
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());
    ESP_ERROR_CHECK(nvs_init_safe());
    ESP_ERROR_CHECK(zb_node_start());
    BaseType_t ok = xTaskCreate(report_task, "zb_report", REPORT_TASK_STACK, NULL,
                                ZB_TASK_PRIO - 1, NULL);
    ESP_ERROR_CHECK(ok == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

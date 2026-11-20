#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"

#include "config.h"
#include "uart_bridge.h"
#include "version.h"
#include "zb_coord.h"

static const char *TAG = "GW";

void app_main(void)
{
    ESP_LOGI(TAG, "firmware=%s", FW_NAME);
    ESP_LOGI(TAG, "version=%s", FW_VERSION);
    ESP_LOGI(TAG, "reset_reason=%d", (int)esp_reset_reason());

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(zb_coord_start());
    ESP_ERROR_CHECK(uart_bridge_start());
}

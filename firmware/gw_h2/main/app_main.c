#include "esp_log.h"
#include "esp_system.h"

#include "config.h"
#include "version.h"

void app_main(void)
{
    ESP_LOGI("ZB", "firmware=%s", FW_NAME);
    ESP_LOGI("ZB", "version=%s", FW_VERSION);
    ESP_LOGI("ZB", "reset_reason=%d", (int)esp_reset_reason());
}

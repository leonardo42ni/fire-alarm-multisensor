#include "esp_log.h"
#include "esp_system.h"

#include "config.h"
#include "version.h"

void app_main(void)
{
    ESP_LOGI("ALM", "firmware=%s", FW_NAME);
    ESP_LOGI("ALM", "version=%s", FW_VERSION);
    ESP_LOGI("ALM", "reset_reason=%d", (int)esp_reset_reason());
}

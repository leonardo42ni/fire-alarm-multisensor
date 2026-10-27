#include "esp_log.h"
#include "esp_system.h"

#include "config.h"
#include "version.h"

void app_main(void)
{
    ESP_LOGI("SNS", "firmware=%s", FW_NAME);
    ESP_LOGI("SNS", "version=%s", FW_VERSION);
    ESP_LOGI("SNS", "reset_reason=%d", (int)esp_reset_reason());
}

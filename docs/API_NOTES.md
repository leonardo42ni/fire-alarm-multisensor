# API NOTES — Chữ ký API đã xác minh (H11)

Mỗi API ESP-IDF/Zigbee/driver được dùng lần đầu phải có một dòng trong bảng dưới.
Chữ ký phải copy nguyên văn từ header đã cài, kèm đường dẫn và số dòng.

| API | Chữ ký thật (copy từ header) | File:dòng | Phiên bản IDF/lib | Task dùng |
|---|---|---|---|---|
| `esp_reset_reason` | `esp_reset_reason_t esp_reset_reason(void);` | `components/esp_system/include/esp_system.h:85` | ESP-IDF v5.5.5 | T-002 |
| `ESP_LOGI` | `#define ESP_LOGI(tag, format, ...) do { ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO, tag, format, ##__VA_ARGS__); } while(0)` | `components/log/include/esp_log.h:127` | ESP-IDF v5.5.5 | T-002 |

## Ghi chú xác minh

- Đã đối chiếu chữ ký với header cài thật tại
  `E:\Espressif\frameworks\esp-idf-v5.5.5\components\esp_system\include\esp_system.h:85`
  (ESP-IDF v5.5.5), khớp với tag `v5.5.1` đã tham chiếu trước đó.
- `ESP_LOGI` có nhiều biến thể theo `__cplusplus`/`ESP_LOG_VERSION`/`NON_OS_BUILD`;
  dòng ghi ở trên là biến thể C thông thường đang dùng (`esp_log.h:127`).
- `esp_reset_reason_t` là enum trong cùng header, gồm `ESP_RST_UNKNOWN`, `ESP_RST_POWERON`,
  `ESP_RST_EXT`, `ESP_RST_SW`, `ESP_RST_PANIC`, `ESP_RST_INT_WDT`, `ESP_RST_TASK_WDT`,
  `ESP_RST_WDT`, `ESP_RST_DEEPSLEEP`, `ESP_RST_BROWNOUT`, `ESP_RST_SDIO`.

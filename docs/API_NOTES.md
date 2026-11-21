# API NOTES — Chữ ký API đã xác minh (H11)

Mỗi API ESP-IDF/Zigbee/driver được dùng lần đầu phải có một dòng trong bảng dưới.
Chữ ký phải copy nguyên văn từ header đã cài, kèm đường dẫn và số dòng.

| API | Chữ ký thật (copy từ header) | File:dòng | Phiên bản IDF/lib | Task dùng |
|---|---|---|---|---|
| `esp_reset_reason` | `esp_reset_reason_t esp_reset_reason(void);` | `components/esp_system/include/esp_system.h:85` | ESP-IDF v5.5.5 | T-002 |
| `ESP_LOGI` | `#define ESP_LOGI(tag, format, ...) do { ESP_LOG_LEVEL_LOCAL(ESP_LOG_INFO, tag, format, ##__VA_ARGS__); } while(0)` | `components/log/include/esp_log.h:127` | ESP-IDF v5.5.5 | T-002 |
| `nvs_flash_init` | `esp_err_t nvs_flash_init(void);` | `components/nvs_flash/include/nvs_flash.h:78` | ESP-IDF v5.5.5 | T-006 |
| `nvs_flash_erase` | `esp_err_t nvs_flash_erase(void);` | `components/nvs_flash/include/nvs_flash.h:147` | ESP-IDF v5.5.5 | T-202 |
| `esp_task_wdt_add` | `esp_err_t esp_task_wdt_add(TaskHandle_t task_handle);` | `components/esp_system/include/esp_task_wdt.h:90` | ESP-IDF v5.5.5 | T-202 |
| `esp_task_wdt_reset` | `esp_err_t esp_task_wdt_reset(void);` | `components/esp_system/include/esp_task_wdt.h:118` | ESP-IDF v5.5.5 | T-202 |
| `esp_zb_platform_config` | `esp_err_t esp_zb_platform_config(esp_zb_platform_config_t *config);` | `managed_components/espressif__esp-zigbee-lib/include/platform/esp_zigbee_platform.h:123` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_init` | `void esp_zb_init(esp_zb_cfg_t *nwk_cfg);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:165` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_set_primary_network_channel_set` | `esp_err_t esp_zb_set_primary_network_channel_set(uint32_t channel_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:178` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_set_pan_id` | `void esp_zb_set_pan_id(uint16_t pan_id);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:270` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_get_pan_id` | `uint16_t esp_zb_get_pan_id(void);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:278` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_get_current_channel` | `uint8_t esp_zb_get_current_channel(void);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:284` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_get_short_address` | `uint16_t esp_zb_get_short_address(void);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:244` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_bdb_start_top_level_commissioning` | `esp_err_t esp_zb_bdb_start_top_level_commissioning(uint8_t mode_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:300` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_bdb_reset_via_local_action` | `void esp_zb_bdb_reset_via_local_action(void);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:309` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_bdb_is_factory_new` | `bool esp_zb_bdb_is_factory_new(void);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:227` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_bdb_open_network` | `esp_err_t esp_zb_bdb_open_network(uint8_t permit_duration);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:252` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_set_rx_on_when_idle` | `void esp_zb_set_rx_on_when_idle(bool rx_on);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:182` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_scheduler_alarm` | `void esp_zb_scheduler_alarm(esp_zb_callback_t cb, uint8_t param, uint32_t time);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:439` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_app_signal_get_params` | `void *esp_zb_app_signal_get_params(uint32_t *signal_p);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:428` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_zdo_signal_to_string` | `const char *esp_zb_zdo_signal_to_string(esp_zb_app_signal_type_t signal);` | `managed_components/espressif__esp-zigbee-lib/include/zdo/esp_zigbee_zdo_command.h:708` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_start` | `esp_err_t esp_zb_start(bool autostart);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:351` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_stack_main_loop` | `void esp_zb_stack_main_loop(void);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:400` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_lock_acquire` | `bool esp_zb_lock_acquire(TickType_t block_ticks);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:369` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_lock_release` | `void esp_zb_lock_release(void);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_core.h:375` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_ep_list_create` | `esp_zb_ep_list_t *esp_zb_ep_list_create(void);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_endpoint.h:20` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_ep_list_add_ep` | `esp_err_t esp_zb_ep_list_add_ep(esp_zb_ep_list_t *ep_list, esp_zb_cluster_list_t *cluster_list, esp_zb_endpoint_config_t endpoint_config);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_endpoint.h:34` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_ep_list_add_gateway_ep` | `esp_err_t esp_zb_ep_list_add_gateway_ep(esp_zb_ep_list_t *ep_list, esp_zb_cluster_list_t *cluster_list, esp_zb_endpoint_config_t endpoint_config);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_endpoint.h:47` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_zcl_cluster_list_create` | `esp_zb_cluster_list_t *esp_zb_zcl_cluster_list_create(void);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:21` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_basic_cluster_create` | `esp_zb_attribute_list_t *esp_zb_basic_cluster_create(esp_zb_basic_cluster_cfg_t *basic_cfg);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:48` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_cluster_list_add_basic_cluster` | `esp_err_t esp_zb_cluster_list_add_basic_cluster(esp_zb_cluster_list_t *cluster_list, esp_zb_attribute_list_t *attr_list, uint8_t role_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:603` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_identify_cluster_create` | `esp_zb_attribute_list_t *esp_zb_identify_cluster_create(esp_zb_identify_cluster_cfg_t *identify_cfg);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:70` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_cluster_list_add_identify_cluster` | `esp_err_t esp_zb_cluster_list_add_identify_cluster(esp_zb_cluster_list_t *cluster_list, esp_zb_attribute_list_t *attr_list, uint8_t role_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:631` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_temperature_meas_cluster_create` | `esp_zb_attribute_list_t *esp_zb_temperature_meas_cluster_create(esp_zb_temperature_meas_cluster_cfg_t *temperature_cfg);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:237` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_cluster_list_add_temperature_meas_cluster` | `esp_err_t esp_zb_cluster_list_add_temperature_meas_cluster(esp_zb_cluster_list_t *cluster_list, esp_zb_attribute_list_t *attr_list, uint8_t role_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:869` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_humidity_meas_cluster_create` | `esp_zb_attribute_list_t *esp_zb_humidity_meas_cluster_create(esp_zb_humidity_meas_cluster_cfg_t *humidity_cfg);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:248` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_cluster_list_add_humidity_meas_cluster` | `esp_err_t esp_zb_cluster_list_add_humidity_meas_cluster(esp_zb_cluster_list_t *cluster_list, esp_zb_attribute_list_t *attr_list, uint8_t role_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:883` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_analog_input_cluster_create` | `esp_zb_attribute_list_t *esp_zb_analog_input_cluster_create(esp_zb_analog_input_cluster_cfg_t *analog_input_cfg);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:371` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_analog_input_cluster_add_attr` | `esp_err_t esp_zb_analog_input_cluster_add_attr(esp_zb_attribute_list_t *attr_list, uint16_t attr_id, void *value_p);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_attribute.h:531` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_cluster_add_attr` | `esp_err_t esp_zb_cluster_add_attr(esp_zb_attribute_list_t *attr_list, uint16_t cluster_id, uint16_t attr_id, uint8_t attr_type, uint8_t attr_access, void *value_p);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_attribute.h:758` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_cluster_list_add_analog_input_cluster` | `esp_err_t esp_zb_cluster_list_add_analog_input_cluster(esp_zb_cluster_list_t *cluster_list, esp_zb_attribute_list_t *attr_list, uint8_t role_mask);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_cluster.h:1054` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_device_register` | `esp_err_t esp_zb_device_register(esp_zb_ep_list_t *ep_list);` | `managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_core.h:349` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_core_action_handler_register` | `void esp_zb_core_action_handler_register(esp_zb_core_action_callback_t cb);` | `managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_core.h:236` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_zcl_set_attribute_val` | `esp_zb_zcl_status_t esp_zb_zcl_set_attribute_val(uint8_t endpoint, uint16_t cluster_id, uint8_t cluster_role, uint16_t attr_id, void *value_p, bool check);` | `managed_components/espressif__esp-zigbee-lib/include/esp_zigbee_attribute.h:60` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_zb_zcl_report_attr_cmd_req` | `esp_err_t esp_zb_zcl_report_attr_cmd_req(esp_zb_zcl_report_attr_cmd_t *cmd_req);` | `managed_components/espressif__esp-zigbee-lib/include/zcl/esp_zigbee_zcl_command.h:2583` | esp-zigbee-lib 1.6.8 | T-006 |
| `esp_timer_get_time` | `int64_t esp_timer_get_time(void);` | `components/esp_timer/include/esp_timer.h:223` | ESP-IDF v5.5.5 | T-006 |
| `esp_err_to_name` | `const char *esp_err_to_name(esp_err_t code);` | `components/esp_common/include/esp_err.h:58` | ESP-IDF v5.5.5 | T-006 |
| `vTaskDelayUntil` | `#define vTaskDelayUntil(pxPreviousWakeTime, xTimeIncrement) do { (void)xTaskDelayUntil((pxPreviousWakeTime), (xTimeIncrement)); } while (0)` (macro, dòng 905–908) | `components/freertos/FreeRTOS-Kernel/include/freertos/task.h:905` | ESP-IDF v5.5.5 | T-006 |
| `xTaskCreate` | `BaseType_t xTaskCreate(TaskFunction_t pxTaskCode, const char * const pcName, const configSTACK_DEPTH_TYPE usStackDepth, void * const pvParameters, UBaseType_t uxPriority, TaskHandle_t * const pxCreatedTask) PRIVILEGED_FUNCTION` | `components/freertos/FreeRTOS-Kernel/include/freertos/task.h:371` | ESP-IDF v5.5.5 | T-006 |
| `uart_param_config` | `esp_err_t uart_param_config(uart_port_t uart_num, const uart_config_t *uart_config);` | `components/esp_driver_uart/include/driver/uart.h:473` | ESP-IDF v5.5.5 | S1-3 |
| `uart_set_pin` | `esp_err_t uart_set_pin(uart_port_t uart_num, int tx_io_num, int rx_io_num, int rts_io_num, int cts_io_num);` | `components/esp_driver_uart/include/driver/uart.h:423` | ESP-IDF v5.5.5 | S1-3 |
| `uart_driver_install` | `esp_err_t uart_driver_install(uart_port_t uart_num, int rx_buffer_size, int tx_buffer_size, int queue_size, QueueHandle_t* uart_queue, int intr_alloc_flags);` | `components/esp_driver_uart/include/driver/uart.h:121` | ESP-IDF v5.5.5 | S1-3 |
| `uart_driver_delete` | `esp_err_t uart_driver_delete(uart_port_t uart_num);` | `components/esp_driver_uart/include/driver/uart.h:132` | ESP-IDF v5.5.5 | S1-3 |
| `uart_write_bytes` | `int uart_write_bytes(uart_port_t uart_num, const void* src, size_t size);` | `components/esp_driver_uart/include/driver/uart.h:533` | ESP-IDF v5.5.5 | S1-3 |
| `uart_read_bytes` | `int uart_read_bytes(uart_port_t uart_num, void* buf, uint32_t length, TickType_t ticks_to_wait);` | `components/esp_driver_uart/include/driver/uart.h:569` | ESP-IDF v5.5.5 | S1-3 |
| `xQueueCreate` | `#define xQueueCreate( uxQueueLength, uxItemSize ) xQueueGenericCreate( ( uxQueueLength ), ( uxItemSize ), ( queueQUEUE_TYPE_BASE ) )` | `components/freertos/FreeRTOS-Kernel/include/freertos/queue.h:149` | ESP-IDF v5.5.5 | S1-3 |
| `xQueueSend` | `#define xQueueSend( xQueue, pvItemToQueue, xTicksToWait ) \` (macro, dòng 469–471) | `components/freertos/FreeRTOS-Kernel/include/freertos/queue.h:469` | ESP-IDF v5.5.5 | S1-3 |
| `xQueueReceive` | `BaseType_t xQueueReceive( QueueHandle_t xQueue, void * const pvBuffer, TickType_t xTicksToWait ) PRIVILEGED_FUNCTION;` | `components/freertos/FreeRTOS-Kernel/include/freertos/queue.h:825` | ESP-IDF v5.5.5 | S1-3 |
| `vQueueDelete` | `void vQueueDelete( QueueHandle_t xQueue ) PRIVILEGED_FUNCTION;` | `components/freertos/FreeRTOS-Kernel/include/freertos/queue.h:864` | ESP-IDF v5.5.5 | S1-3 |
| `vTaskDelete` | `void vTaskDelete( TaskHandle_t xTaskToDelete ) PRIVILEGED_FUNCTION;` | `components/freertos/FreeRTOS-Kernel/include/freertos/task.h:785` | ESP-IDF v5.5.5 | S1-3 |
| `esp_zb_ieee_address_by_short` | `esp_err_t esp_zb_ieee_address_by_short(uint16_t short_addr, uint8_t *ieee_addr);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:304` | esp-zigbee-lib 1.6.8 | S1-3 |
| `esp_zb_address_short_by_ieee` | `uint16_t esp_zb_address_short_by_ieee(esp_zb_ieee_addr_t address);` | `managed_components/espressif__esp-zigbee-lib/include/nwk/esp_zigbee_nwk.h:293` | esp-zigbee-lib 1.6.8 | S1-3 |
| `gpio_config` | `esp_err_t gpio_config(const gpio_config_t *pGPIOConfig);` | `components/esp_driver_gpio/include/driver/gpio.h:71` | ESP-IDF v5.5.5 | S1-4 |
| `gpio_set_level` | `esp_err_t gpio_set_level(gpio_num_t gpio_num, uint32_t level);` | `components/esp_driver_gpio/include/driver/gpio.h:141` | ESP-IDF v5.5.5 | S1-4 |

## Kiểu dữ liệu / hằng số đã xác minh

| Tên | Định nghĩa / vị trí | File:dòng | Task dùng |
|---|---|---|---|
| `esp_zb_cfg_t` | struct gồm `esp_zb_role`, `install_code_policy`, `nwk_cfg.zczr_cfg` / `nwk_cfg.zed_cfg` | `esp_zigbee_core.h:81` | T-006 |
| `esp_zb_zed_cfg_t` | `typedef struct { uint8_t ed_timeout; uint32_t keep_alive; } esp_zb_zed_cfg_t;` | `esp_zigbee_core.h:72` | T-202 |
| `ESP_ERR_NVS_NO_FREE_PAGES` | `#define ESP_ERR_NVS_NO_FREE_PAGES (ESP_ERR_NVS_BASE + 0x0d)` | `components/nvs_flash/include/nvs.h:42` | T-202 |
| `ESP_ERR_NVS_NEW_VERSION_FOUND` | `#define ESP_ERR_NVS_NEW_VERSION_FOUND (ESP_ERR_NVS_BASE + 0x10)` | `components/nvs_flash/include/nvs.h:46` | T-202 |
| `esp_zb_platform_config_t` | struct gồm `radio_config`, `host_config` | `platform/esp_zigbee_platform.h:80` | T-006 |
| `esp_zb_core_action_callback_t` | `typedef esp_err_t (*esp_zb_core_action_callback_t)(esp_zb_core_action_callback_id_t callback_id, const void *message);` | `zcl/esp_zigbee_zcl_core.h:228` | T-006 |
| `ESP_ZB_CORE_REPORT_ATTR_CB_ID` | `= 0x2000` | `zcl/esp_zigbee_zcl_core.h:178` | T-006 |
| `esp_zb_zcl_report_attr_message_t` | struct gồm `status`, `src_address`, `src_endpoint`, `dst_endpoint`, `cluster`, `attribute` | `zcl/esp_zigbee_zcl_command.h:2173` | T-006 |
| `esp_zb_zcl_report_attr_cmd_t` | struct gồm `zcl_basic_cmd`, `address_mode`, `clusterID`, `manuf_specific`, `direction`, `dis_default_resp`, `manuf_code`, `attributeID` | `zcl/esp_zigbee_zcl_command.h:169` | T-006 |
| `ESP_ZB_ZCL_CLUSTER_SERVER_ROLE` | `= 0x01U` | `zcl/esp_zigbee_zcl_common.h:89` | T-006 |
| `ESP_ZB_ZCL_CLUSTER_ID_ANALOG_INPUT` | `= 0x000cU` | `zcl/esp_zigbee_zcl_common.h:110` | T-006 |
| `ESP_ZB_ZCL_CLUSTER_ID_TEMP_MEASUREMENT` | `= 0x0402U` | `zcl/esp_zigbee_zcl_common.h:135` | T-006 |
| `ESP_ZB_ZCL_CLUSTER_ID_REL_HUMIDITY_MEASUREMENT` | `= 0x0405U` | `zcl/esp_zigbee_zcl_common.h:138` | T-006 |
| `ESP_ZB_ZCL_CMD_DIRECTION_TO_CLI` | `= 0x01U` | `zcl/esp_zigbee_zcl_common.h:281` | T-006 |
| `ESP_ZB_ZCL_ATTR_NON_MANUFACTURER_SPECIFIC` | `0xFFFFU` | `zcl/esp_zigbee_zcl_common.h:23` | T-006 |
| `ESP_ZB_APS_ADDR_MODE_16_ENDP_PRESENT` | `= 0x2` | `aps/esp_zigbee_aps.h:23` | T-006 |
| `ESP_ZB_ZCL_ATTR_TEMP_MEASUREMENT_VALUE_ID` | `= 0x0000` | `zcl/esp_zigbee_zcl_temperature_meas.h:17` | T-006 |
| `ESP_ZB_ZCL_ATTR_REL_HUMIDITY_MEASUREMENT_VALUE_ID` | `= 0x0000` | `zcl/esp_zigbee_zcl_humidity_meas.h:17` | T-006 |
| `ESP_ZB_ZCL_ATTR_ANALOG_INPUT_PRESENT_VALUE_ID` | `= 0x0055` | `zcl/esp_zigbee_zcl_analog_input.h:20` | T-006 |
| `ESP_ZB_ZCL_ATTR_ANALOG_INPUT_DESCRIPTION_ID` | `= 0x001c` | `zcl/esp_zigbee_zcl_analog_input.h:16` | T-006 |
| `ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID` | `= 0x0100` | `zcl/esp_zigbee_zcl_analog_input.h:25` | T-006 |
| `ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK` | `0x07FFF800U` (channel 11–26) | `esp_zigbee_core.h:27` | T-006 |
| `ESP_ZB_ZCL_ATTR_TYPE_U16` | `= 0x21U` | `zcl/esp_zigbee_zcl_common.h:219` | T-006 |
| `ESP_ZB_ZCL_ATTR_TYPE_U32` | `= 0x23U` | `zcl/esp_zigbee_zcl_common.h:221` | T-006 |
| `ESP_ZB_ZCL_ATTR_ACCESS_READ_ONLY` | `= 0x01U` | `zcl/esp_zigbee_zcl_common.h:265` | T-006 |
| `ESP_ZB_ZCL_ATTR_ACCESS_REPORTING` | `= 0x04U` | `zcl/esp_zigbee_zcl_common.h:268` | T-006 |
| `ESP_ZB_ZCL_ATTR_TYPE_S16` | `= 0x29U` | `zcl/esp_zigbee_zcl_common.h:227` | T-006 |
| `ESP_ZB_ZCL_ATTR_TYPE_SINGLE` | `= 0x39U` | `zcl/esp_zigbee_zcl_common.h:237` | T-006 |
| `esp_zb_app_signal_t` | `typedef struct esp_zb_app_signal_s { uint32_t *p_app_signal; esp_err_t esp_err_status; } esp_zb_app_signal_t;` | `esp_zigbee_core.h:94` | T-006 |
| `esp_zb_app_signal_type_t` | `typedef enum { ... } esp_zb_app_signal_type_t;` (enum tín hiệu ứng dụng; các giá trị dùng ghi ở bảng hằng số bên dưới) | `zdo/esp_zigbee_zdo_common.h:46` | T-006 |
| `esp_zb_callback_t` | `typedef void (*esp_zb_callback_t)(uint8_t param);` | `esp_zigbee_type.h:23` | T-006 |
| `esp_zb_core_action_callback_id_t` | `typedef enum esp_zb_core_action_callback_id_s { ... } esp_zb_core_action_callback_id_t;` (enum callback ID; dùng `ESP_ZB_CORE_REPORT_ATTR_CB_ID`) | `zcl/esp_zigbee_zcl_core.h:94` | T-006 |
| `esp_zb_zdo_signal_device_annce_params_t` | `typedef struct esp_zb_zdo_signal_device_annce_params_s { uint16_t device_short_addr; esp_zb_ieee_addr_t ieee_addr; uint8_t capability; } esp_zb_zdo_signal_device_annce_params_t;` | `zdo/esp_zigbee_zdo_common.h:681` | T-006 |
| `esp_zb_zcl_attribute_t` | `typedef struct esp_zb_zcl_attribute_s { uint16_t id; esp_zb_zcl_attribute_data_t data; } esp_zb_zcl_attribute_t;`; `data.type`, `data.value` thuộc `esp_zb_zcl_attribute_data_t` tại `zcl/esp_zigbee_zcl_common.h:297` | `zcl/esp_zigbee_zcl_common.h:307` | T-006 |
| `esp_zb_zcl_status_t` | `typedef enum { ... } esp_zb_zcl_status_t;` (enum trạng thái ZCL; `ESP_ZB_ZCL_STATUS_SUCCESS`/`FAIL` ghi ở bảng hằng số) | `zcl/esp_zigbee_zcl_common.h:160` | T-006 |
| `esp_zb_endpoint_config_t` | `typedef struct esp_zb_endpoint_config_s { uint8_t endpoint; uint16_t app_profile_id; uint16_t app_device_id; uint32_t app_device_version: 4; } esp_zb_endpoint_config_t;` | `esp_zigbee_type.h:350` | T-006 |
| `esp_zb_zcl_basic_cmd_t` | `typedef struct esp_zb_zcl_basic_cmd_s { esp_zb_addr_u dst_addr_u; uint8_t dst_endpoint; uint8_t src_endpoint; } esp_zb_zcl_basic_cmd_t;`; `dst_addr_u.addr_short` thuộc `esp_zb_addr_u` tại `esp_zigbee_type.h:32` | `zcl/esp_zigbee_zcl_common.h:326` | T-006 |
| `esp_zb_basic_cluster_cfg_t` | `typedef struct esp_zb_basic_cluster_cfg_s { uint8_t zcl_version; uint8_t power_source; } esp_zb_basic_cluster_cfg_t;` | `esp_zigbee_type.h:402` | T-006 |
| `esp_zb_identify_cluster_cfg_t` | `typedef struct esp_zb_identify_cluster_cfg_s { uint16_t identify_time; } esp_zb_identify_cluster_cfg_t;` | `esp_zigbee_type.h:424` | T-006 |
| `esp_zb_temperature_meas_cluster_cfg_t` | `typedef struct esp_zb_temperature_meas_cluster_cfg_s { int16_t measured_value; int16_t min_value; int16_t max_value; } esp_zb_temperature_meas_cluster_cfg_t;` | `esp_zigbee_type.h:616` | T-006 |
| `esp_zb_humidity_meas_cluster_cfg_t` | `typedef struct esp_zb_humidity_meas_cluster_cfg_s { uint16_t measured_value; uint16_t min_value; uint16_t max_value; } esp_zb_humidity_meas_cluster_cfg_t;` | `esp_zigbee_type.h:626` | T-006 |
| `esp_zb_analog_input_cluster_cfg_t` | `typedef struct esp_zb_analog_input_cluster_cfg_s { bool out_of_service; float_t present_value; uint8_t status_flags; } esp_zb_analog_input_cluster_cfg_t;` | `esp_zigbee_type.h:739` | T-006 |
| `esp_zb_cluster_list_t` | `typedef struct esp_zb_cluster_list_s { esp_zb_zcl_cluster_t cluster; struct esp_zb_cluster_list_s *next; } esp_zb_cluster_list_t;` | `esp_zigbee_type.h:149` | T-006 |
| `esp_zb_ep_list_t` | `typedef struct esp_zb_ep_list_s { esp_zb_endpoint_t endpoint; struct esp_zb_ep_list_s *next; } esp_zb_ep_list_t;` | `esp_zigbee_type.h:392` | T-006 |
| `esp_zb_zcl_cluster_list_t` | **Không tồn tại trong esp-zigbee-lib 1.6.8**; kiểu thật là `esp_zb_cluster_list_t` (dòng 149). Hàm `esp_zb_zcl_cluster_list_create()` trả `esp_zb_cluster_list_t *`. | — | T-006 |
| `ESP_ZB_ZCL_STATUS_SUCCESS` | `= 0x00U` | `zcl/esp_zigbee_zcl_common.h:161` | T-006 |
| `ESP_ZB_ZCL_STATUS_FAIL` | `= 0x01U` | `zcl/esp_zigbee_zcl_common.h:162` | T-006 |
| `ESP_ZB_BDB_MODE_INITIALIZATION` | `= ESP_ZB_BDB_INITIALIZATION` (`ESP_ZB_BDB_INITIALIZATION = 0x00`) | `bdb/esp_zigbee_bdb_commissioning.h:35` | T-006 |
| `ESP_ZB_BDB_MODE_NETWORK_FORMATION` | `= ESP_ZB_BDB_NETWORK_FORMATION` (`ESP_ZB_BDB_NETWORK_FORMATION = 0x04`) | `bdb/esp_zigbee_bdb_commissioning.h:38` | T-006 |
| `ESP_ZB_BDB_MODE_NETWORK_STEERING` | `= ESP_ZB_BDB_NETWORK_STEERING` (`ESP_ZB_BDB_NETWORK_STEERING = 0x02`) | `bdb/esp_zigbee_bdb_commissioning.h:37` | T-006 |
| `ESP_ZB_BDB_SIGNAL_DEVICE_FIRST_START` | `= 0x05` | `zdo/esp_zigbee_zdo_common.h:140` | T-006 |
| `ESP_ZB_BDB_SIGNAL_DEVICE_REBOOT` | `= 0x06` | `zdo/esp_zigbee_zdo_common.h:156` | T-006 |
| `ESP_ZB_BDB_SIGNAL_FORMATION` | `= 0x0b` | `zdo/esp_zigbee_zdo_common.h:235` | T-006 |
| `ESP_ZB_BDB_SIGNAL_STEERING` | `= 0x0a` | `zdo/esp_zigbee_zdo_common.h:219` | T-006 |
| `ESP_ZB_ZDO_SIGNAL_SKIP_STARTUP` | `= 0x01` | `zdo/esp_zigbee_zdo_common.h:78` | T-006 |
| `ESP_ZB_ZDO_SIGNAL_DEVICE_ANNCE` | `= 0x02` | `zdo/esp_zigbee_zdo_common.h:93` | T-006 |
| `ESP_ZB_ZDO_SIGNAL_LEAVE` | `= 0x03` | `zdo/esp_zigbee_zdo_common.h:108` | T-006 |
| `ESP_ZB_NWK_SIGNAL_NO_ACTIVE_LINKS_LEFT` | `= 0x18` | `zdo/esp_zigbee_zdo_common.h:410` | T-006 |
| `ESP_ZB_NWK_SIGNAL_PERMIT_JOIN_STATUS` | `= 0x36` | `zdo/esp_zigbee_zdo_common.h:513` | T-006 |
| `ESP_ZB_DEVICE_TYPE_ED` | `= 0x2` | `nwk/esp_zigbee_nwk.h:20` | T-006 |
| `ESP_ZB_DEVICE_TYPE_COORDINATOR` | `= 0x0` | `nwk/esp_zigbee_nwk.h:18` | T-006 |
| `ESP_ZB_ED_AGING_TIMEOUT_64MIN` | `= 6U` | `esp_zigbee_core.h:49` | T-006 |
| `ZB_RADIO_MODE_NATIVE` | `= 0x0` | `platform/esp_zigbee_platform.h:22` | T-006 |
| `ZB_HOST_CONNECTION_MODE_NONE` | `= 0x0` | `platform/esp_zigbee_platform.h:30` | T-006 |
| `ESP_ZB_AF_HA_PROFILE_ID` | `= 0x0104U` | `zcl/esp_zigbee_zcl_common.h:35` | T-006 |
| `ESP_ZB_HA_HOME_GATEWAY_DEVICE_ID` | `= 0x0050` | `zcl/esp_zigbee_zcl_common.h:58` | T-006 |
| `ESP_ZB_HA_TEMPERATURE_SENSOR_DEVICE_ID` | `= 0x0302` | `zcl/esp_zigbee_zcl_common.h:74` | T-006 |
| `ESP_ZB_HA_CUSTOM_ATTR_DEVICE_ID` | `= 0xfff2` | `zcl/esp_zigbee_zcl_common.h:81` | T-006 |
| `ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE` | `#define ESP_ZB_ZCL_BASIC_ZCL_VERSION_DEFAULT_VALUE ((uint8_t)0x08)` | `zcl/esp_zigbee_zcl_basic.h:58` | T-006 |
| `ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE` | `#define ESP_ZB_ZCL_BASIC_POWER_SOURCE_DEFAULT_VALUE ((uint8_t)0x00)` | `zcl/esp_zigbee_zcl_basic.h:79` | T-006 |
| `ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE` | `#define ESP_ZB_ZCL_IDENTIFY_IDENTIFY_TIME_DEFAULT_VALUE 0x0000` | `zcl/esp_zigbee_zcl_identify.h:21` | T-006 |
| `ESP_ZB_ZCL_TEMP_MEASUREMENT_MIN_MEASURED_VALUE_MINIMUM` | `#define ... ((int16_t)0x954d)` | `zcl/esp_zigbee_zcl_temperature_meas.h:24` | T-006 |
| `ESP_ZB_ZCL_TEMP_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM` | `#define ... ((int16_t)0x7fff)` | `zcl/esp_zigbee_zcl_temperature_meas.h:33` | T-006 |
| `ESP_ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MIN_MEASURED_VALUE_MINIMUM` | `#define ... ((uint16_t)0x0000)` | `zcl/esp_zigbee_zcl_humidity_meas.h:24` | T-006 |
| `ESP_ZB_ZCL_REL_HUMIDITY_MEASUREMENT_MAX_MEASURED_VALUE_MAXIMUM` | `#define ... ((uint16_t)0x2710)` | `zcl/esp_zigbee_zcl_humidity_meas.h:33` | T-006 |
| `uart_config_t` | struct gồm `baud_rate`, `data_bits`, `parity`, `stop_bits`, `flow_ctrl`, `rx_flow_ctrl_thresh`, union `source_clk`/`lp_source_clk`, `flags` | `components/esp_driver_uart/include/driver/uart.h:35` | S1-3 |
| `UART_NUM_1` | `UART_NUM_1,` (enum `uart_port_t`) | `components/hal/include/hal/uart_types.h:24` | S1-3 |
| `UART_DATA_8_BITS` | `= 0x3` | `components/hal/include/hal/uart_types.h:60` | S1-3 |
| `UART_STOP_BITS_1` | `= 0x1` | `components/hal/include/hal/uart_types.h:68` | S1-3 |
| `UART_PARITY_DISABLE` | `= 0x0` | `components/hal/include/hal/uart_types.h:78` | S1-3 |
| `UART_HW_FLOWCTRL_DISABLE` | `= 0x0` | `components/hal/include/hal/uart_types.h:87` | S1-3 |
| `UART_SCLK_DEFAULT` | `= SOC_MOD_CLK_PLL_F48M` | `components/soc/esp32h2/include/soc/clk_tree_defs.h:252` | S1-3 |
| `UART_PIN_NO_CHANGE` | `#define UART_PIN_NO_CHANGE (-1)` | `components/esp_driver_uart/include/driver/uart.h:23` | S1-3 |
| `portMUX_TYPE` | `typedef spinlock_t portMUX_TYPE;` | `components/freertos/FreeRTOS-Kernel/portable/riscv/include/freertos/portmacro.h:201` | S1-3 |
| `portMUX_INITIALIZER_UNLOCKED` | `#define portMUX_INITIALIZER_UNLOCKED SPINLOCK_INITIALIZER` | `components/freertos/FreeRTOS-Kernel/portable/riscv/include/freertos/portmacro.h:203` | S1-3 |
| `taskENTER_CRITICAL` | `#define taskENTER_CRITICAL( x ) portENTER_CRITICAL( x )` | `components/freertos/FreeRTOS-Kernel/include/freertos/task.h:236` | S1-3 |
| `taskEXIT_CRITICAL` | `#define taskEXIT_CRITICAL( x ) portEXIT_CRITICAL( x )` | `components/freertos/FreeRTOS-Kernel/include/freertos/task.h:250` | S1-3 |
| `pdMS_TO_TICKS` | `#define pdMS_TO_TICKS( xTimeInMs ) ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * ( TickType_t ) configTICK_RATE_HZ ) / ( TickType_t ) 1000U ) )` | `components/freertos/FreeRTOS-Kernel/include/freertos/projdefs.h:46` | S1-3 |
| `esp_zb_zcl_addr_t` | struct `{ uint8_t addr_type; union { uint16_t short_addr; uint32_t src_id; esp_zb_ieee_addr_t ieee_addr; } u; }` | `esp_zigbee_type.h:80` | S1-3 |
| `ESP_ZB_ZCL_ADDR_TYPE_SHORT` | `= 0U` | `esp_zigbee_type.h:41` | S1-3 |
| `ESP_ZB_ZDO_SIGNAL_LEAVE_INDICATION` | `= 0x13` | `zdo/esp_zigbee_zdo_common.h:346` | S1-3 |
| `esp_zb_zdo_signal_leave_indication_params_t` | struct `{ uint16_t short_addr; esp_zb_ieee_addr_t device_addr; uint8_t rejoin; }` | `zdo/esp_zigbee_zdo_common.h:662` | S1-3 |
| `esp_zb_zczr_cfg_t` | `typedef struct { uint8_t max_children; } esp_zb_zczr_cfg_t;` | `esp_zigbee_core.h:64` | S1-3 |
| `esp_zb_ieee_addr_t` | `typedef esp_zb_64bit_addr_t esp_zb_ieee_addr_t;` (`uint8_t[8]`, little-endian: `[0]`=LSB, `[7]`=MSB) | `esp_zigbee_type.h:20` | S1-3 |
| `esp_zb_zcl_attribute_data_t` | struct `{ esp_zb_zcl_attr_type_t type; uint16_t size; void *value; } ESP_ZB_PACKED_STRUCT` | `zcl/esp_zigbee_zcl_common.h:297` | S1-3 |
| `ESP_ZB_ZCL_ADDR_TYPE_IEEE` | `= 3U` | `esp_zigbee_type.h:44` | S1-3 |
| `ESP_ZB_ZDO_SIGNAL_DEVICE_UPDATE` | `= 0x30` | `zdo/esp_zigbee_zdo_common.h:447` | S1-3 |
| `esp_zb_zdo_signal_device_update_params_t` | struct `{ esp_zb_ieee_addr_t long_addr; uint16_t short_addr; uint8_t status; uint8_t tc_action; uint16_t parent_short; }` | `zdo/esp_zigbee_zdo_common.h:690` | S1-3 |
| `ESP_ZB_ZDO_STANDARD_DEV_LEFT` | `= 0x02U` (enum `esp_zb_zdo_update_dev_status_t`) | `zdo/esp_zigbee_zdo_common.h:645` | S1-3 |
| `gpio_config_t` | struct gồm `pin_bit_mask`, `mode`, `pull_up_en`, `pull_down_en`, `intr_type` | `components/esp_driver_gpio/include/driver/gpio.h:46` | S1-4 |
| `GPIO_MODE_OUTPUT` | `= GPIO_MODE_DEF_OUTPUT` (enum `gpio_mode_t`) | `components/hal/include/hal/gpio_types.h:100` | S1-4 |
| `GPIO_PULLUP_DISABLE` | `= 0x0` (enum `gpio_pullup_t`) | `components/hal/include/hal/gpio_types.h:107` | S1-4 |
| `GPIO_PULLDOWN_DISABLE` | `= 0x0` (enum `gpio_pulldown_t`) | `components/hal/include/hal/gpio_types.h:112` | S1-4 |
| `GPIO_INTR_DISABLE` | `= 0` (enum `gpio_int_type_t`) | `components/hal/include/hal/gpio_types.h:81` | S1-4 |
| `UART_NUM_2` | `UART_NUM_2` (enum `uart_port_t`) | `components/hal/include/hal/uart_types.h:26` | S1-4 |
| `UART_SCLK_DEFAULT` (ESP32) | `= SOC_MOD_CLK_APB` | `components/soc/esp32/include/soc/clk_tree_defs.h:232` | S1-4 |

## Ghi chú xác minh

- Đã đối chiếu chữ ký với header cài thật tại
  `E:\Espressif\frameworks\esp-idf-v5.5.5\components\esp_system\include\esp_system.h:85`
  (ESP-IDF v5.5.5), khớp với tag `v5.5.1` đã tham chiếu trước đó.
- `ESP_LOGI` có nhiều biến thể theo `__cplusplus`/`ESP_LOG_VERSION`/`NON_OS_BUILD`;
  dòng ghi ở trên là biến thể C thông thường đang dùng (`esp_log.h:127`).
- `esp_reset_reason_t` là enum trong cùng header, gồm `ESP_RST_UNKNOWN`, `ESP_RST_POWERON`,
  `ESP_RST_EXT`, `ESP_RST_SW`, `ESP_RST_PANIC`, `ESP_RST_INT_WDT`, `ESP_RST_TASK_WDT`,
  `ESP_RST_WDT`, `ESP_RST_DEEPSLEEP`, `ESP_RST_BROWNOUT`, `ESP_RST_SDIO`.
- `app_err_t` (`firmware/common/error.h`) là kiểu nội bộ, không alias sang `esp_err_t`;
  mọi chuyển đổi phải tường minh.
- Các macro `ESP_ZB_ZED_CONFIG()`, `ESP_ZB_ZC_CONFIG()`, `ESP_ZB_DEFAULT_RADIO_CONFIG()`,
  `ESP_ZB_DEFAULT_HOST_CONFIG()` **không** có trong esp-zigbee-lib 1.6.8; đây là macro cục bộ
  trong `firmware/node_h2/main/app_main.c` và `firmware/gw_h2/main/app_main.c`, lấy theo mẫu ESP-IDF.
- Coordinator dùng `esp_zb_ep_list_add_gateway_ep()` cho EP1 để bỏ qua kiểm tra cluster data model khi
  nhận report từ các cluster không khai báo trên coordinator (theo mẫu `esp_zigbee_gateway` và tài liệu
  ESP Zigbee SDK về gateway endpoint).
- Dependency lock thực tế: `espressif/esp-zigbee-lib 1.6.8`, `espressif/esp-zboss-lib 1.6.4`,
  target `esp32h2`, ESP-IDF v5.5.5.
- Đã build thành công `node_h2` (0x6e040) và `gw_h2` (0x90190) với target `esp32h2`; đã kiểm chứng runtime G0
  trên hai board ESP32-H2 ngày 2026-11-12.
- **B3/G0 spike:** `0x0056` không có trong enum analog input, nên dùng
  `ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID` (`0x0100`) trên `EP_FLAGS` làm nơi mang `seq`.
  Disassembly `libesp_zb_api.ed.debug.a` cho thấy `esp_zb_analog_input_cluster_add_attr()` gán attr
  `0x0100` kiểu `U32` (`0x23`), nên node gửi `uint32_t` và gateway đọc `uint32_t`. Đây là quy ước
  riêng cho spike G0, không dùng làm dữ liệu nghiệp vụ.
- **B3 report fix:** `esp_zb_analog_input_cluster_add_attr()` thêm ApplicationType ở dạng read-only,
  nên `esp_zb_zcl_report_attr_cmd_req()` trả `FAIL`. Node chuyển sang `esp_zb_cluster_add_attr()`
  với `READ_ONLY | REPORTING`; HIL xác nhận lại report `0x0100` thành công.
- **B5/Zigbee Kconfig đã đối chiếu Kconfig thật:**
  `CONFIG_ZB_ENABLED`, `CONFIG_ZB_RADIO_NATIVE`, `CONFIG_ZB_ZED` (node),
  `CONFIG_ZB_ZCZR` (gateway). Partition: `zb_storage` 16K, `zb_fct` 1K, `factory` 1500K.
- **R2:** node dùng `ESP_ZB_TRANSCEIVER_ALL_CHANNELS_MASK` thay vì ép kênh đơn; coordinator vẫn
  dùng `1UL << ZB_CHANNEL` để cố định kênh 25 cho spike.
- **G0 HIL 2026-11-12:** B1 factory-new PASS (`tests/results/g0_gw_20261112.log`,
  `g0_node_20261112.log`; `serial_check.py --input` PASS), S2 = 2866 ms; B3/S4 `seq=1..1000`,
  nhận 992, thiếu 8, mất 0,8 % (`tests/results/g0_seq_20261112.log`).
- **S5 HIL 2026-11-12:** node-only `g0_s5c_gw_20261112.log` gap 15,839 s; GW+node power-cycle
  `g0_s5d_gw_20261112.log` + `g0_s5d_node_20261112.log`: GW boot → report đầu 3,337 s,
  node `REJOINED` 2,466 s; cả hai < 60 s. Lần GW-only thứ hai không capture được do cổng serial
  không reopen; phép đo đồng thời được dùng làm bằng chứng S5 và ghi rõ caveat.
- **T-201/N1:** `proto_enc_rpt` luôn in `rssi`/`lqi`; giá trị `0` nghĩa là "không rõ".
  Nếu §5.1 sau này yêu cầu bỏ qua khi không rõ thì thêm bit `valid` tương ứng.
- **T-201/N4:** `flat_get_num` giả định đầu vào đã qua `proto_json_valid_object`
  (dùng trực tiếp `strtod` sẽ nhận cả chuỗi hex kiểu `0x10`).
- **T-202:** `node_snapshot_t` hiện khai báo trong `firmware/node_h2/components/zb_node/include/zb_node.h`
  đúng §5.3.6; T-106 sẽ chuyển sang `node_state.h` khi component `node_state` được làm.
- **T-202:** giữ quy ước G0/B3 gửi `seq` qua `ESP_ZB_ZCL_ATTR_ANALOG_INPUT_APPLICATION_TYPE_ID`
  (`0x0100`, kiểu U32) trên EP15 để Gateway tính PDR; đây không phải trường nghiệp vụ mới.
- **T-202/C2:** `esp_zb_bdb_reset_via_local_action()` chỉ gọi trong `reset_local_cb` (Zigbee task).
  Mọi yêu cầu reset từ task ứng dụng đi qua `zb_node_request_reset_app()`: lấy
  `esp_zb_lock_acquire()` rồi mới gọi `esp_zb_scheduler_alarm()`, sau đó nhả khóa.
- **T-202/C8:** EP11–15 chỉ gồm Basic + Analog Input `0x000C` + `Description`, **không** có Identify,
  đúng §5.3.7 (Identify chỉ nằm trên EP10).
- **T-202/C9:** `flame_do`/`prealert` được OR vào bit 3/4 của `flags` trước khi gửi EP15,
  dùng `ZB_NODE_FLAG_FLAME_DO`/`ZB_NODE_FLAG_PREALERT` trong `zb_node_config.h` (khớp GatewayPlan §4.3).
- **T-202/R3:** `CONFIG_ESP_TASK_WDT_TIMEOUT_S=10` > `SPIKE_REPORT_MS=3000`; khi chuyển sang
  `REPORT_NORMAL_MS=10000` phải giữ timeout TWDT > 10 s hoặc chia nhỏ chu kỳ wake.
- **T-202 HIL 2026-11-20:** Phase A PASS: node `REJOINED|t_ms=2408`, GW `JOIN|SA=0x46a9`,
  8 report `seq` trong 30 s, giá trị khớp `SPIKE_*`. S5 (GW off 95 s) PASS: node
  `ZB_SEND|FAIL`, stack `START|FAIL` backoff 5→10→20→40 s, rejoin sau 16,6 s khi GW bật lại.
  S5b (GW off 115 s) PASS: backoff đạt `retry_ms=60000`, node `REJOINED` sau 58,2 s và report
  đầu tiên sau 59,61 s (<60 s). Log `tests/results/g0_t202_join_*_20261120.log`,
  `g0_s5_backoff_*_20261120.log`, `g0_s5b_backoff_*_20261120.log`. HIL đi qua nhánh stack
  `START|FAIL` (mất parent), chưa kích hoạt riêng nhánh `tx_fail=3`/`REJOIN|REQUEST`;
  C1–C3 đã được review code, không thay đổi sau HIL.
- **S1-3 (2026-11-20):** `gw_h2` tách thành đúng 2 component `zb_coord` + `uart_bridge`
  (mỗi component 1 cặp `.c/.h`), `main/app_main.c` chỉ còn `nvs_flash_init()` →
  `zb_coord_start()` → `uart_bridge_start()`.
- **S1-3 UART:** `UART_NUM_1`, TX=GPIO10, RX=GPIO11, 115200 8N1, RX/TX buffer 1024;
  log vẫn đi USB-Serial-JTAG (`CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`) nên không lẫn vào UART1.
- **S1-3 Zigbee callback:** `ESP_ZB_CORE_REPORT_ATTR_CB_ID` kiểm `attribute.data.value != NULL` và `attribute.data.size`, dùng `memcpy` từ `ESP_ZB_PACKED_STRUCT` (`S16/U16/U32/SINGLE`) vào `zb_evt_t` rồi `xQueueSend(..., 0)`; mọi UART/`ESP_LOGx` nằm ở task `uart_bridge`. Hỗ trợ cả `ESP_ZB_ZCL_ADDR_TYPE_SHORT` (kèm fallback `esp_zb_ieee_address_by_short` khi GW reboot mà Node vẫn còn trong NVS) và `ESP_ZB_ZCL_ADDR_TYPE_IEEE` (`esp_zb_address_short_by_ieee`). `LEAVE_INDICATION` (`0x13`) và `DEVICE_UPDATE` (`0x30`, `DEV_LEFT`, khử trùng qua cache) xóa cache và gửi `leave`; `ESP_ZB_ZDO_SIGNAL_LEAVE` xóa sạch cache và log.
- **S1-3 IEEE byte order:** Đã xác minh trong `nwk/esp_zigbee_nwk.h:221,299`, `zb_address.h:140-143`, `zb_trace.h:513,528`: `esp_zb_ieee_addr_t` là little-endian (`[0]`=LSB, `[7]`=MSB), nên format `[7]..[0]` ra 16 hex MSB-first khớp `proto_ndjson.h`.
- **S1-3 build & COM5 boot:** `.\tools\build_all.ps1` PASS cả 3 firmware (`gw_h2.bin` `0x9b710`, 59% free, không warning); `ctest` 5/5 PASS; flash & monitor USB-Serial-JTAG `COM5` PASS (`FORMED|PAN=0x1a62|CH=25|SA=0x0000|PASS`, `PERMIT|OPEN|sec=180`, `START|UART1|TX=10|RX=11`). DoD 1 giờ với Node + đo trực tiếp UART1 (`GPIO10`/`GPIO11`) đang chờ gắn đủ 2 board / `gw_host`.
- **S1-4 (2026-11-21):** `gw_host` tách component `uart_link` (`UART_NUM_2`, RX=GPIO16, TX=GPIO17, 115200 8N1, buffer 1024); `app_main()` kéo LOW `BUZZER_GPIO` (GPIO4) và `RGB` (`GPIO25/26/27`) ngay đầu hàm (R7); bỏ `PWR_SENSE` khỏi `config.h`; `tools/log_csv.py` ghi `CSV,...` + `pc_time,run_id,label,event` vào `data/raw/<run_id>.csv`.

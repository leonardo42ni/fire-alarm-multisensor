#ifndef UART_LINK_H
#define UART_LINK_H

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "proto_ndjson.h"
#include "shared_config.h"

#define UART_LINK_TX_GPIO 17
#define UART_LINK_RX_GPIO 16
#define UART_LINK_RX_CHUNK 128
#define UART_LINK_READ_TIMEOUT_MS 100u
#define UART_LINK_TASK_STACK 4096
#define UART_LINK_TASK_PRIO 5
#define UART_LINK_Q_RPT_LEN 8
#define UART_LINK_MAX_NODES 4
#define UART_LINK_NODE_TIMEOUT_MS NODE_OFFLINE_MIN_MS
#define UART_LINK_MS_PER_S 1000u

esp_err_t uart_link_start(void);
QueueHandle_t uart_link_rpt_queue(void);
esp_err_t uart_link_send_cmd(cmd_op_t op, uint16_t sec);
bool uart_link_h2_online(void);
bool uart_link_any_node_online(void);
void uart_link_print_csv(const rpt_t *r, uint32_t t_ms);

#endif /* UART_LINK_H */

#ifndef UART_BRIDGE_H
#define UART_BRIDGE_H

#include "esp_err.h"

#define UART_BRIDGE_TASK_STACK 4096
#define UART_BRIDGE_TASK_PRIO 4
#define UART_BRIDGE_TICK_MS 50u
#define UART_BRIDGE_MAX_NODES 4
#define UART_BRIDGE_PERMIT_MIN_S 1u
#define UART_BRIDGE_PERMIT_MAX_S 254u
#define UART_BRIDGE_TX_GPIO 10
#define UART_BRIDGE_RX_GPIO 11
#define UART_BRIDGE_RX_CHUNK 128
#define UART_BRIDGE_MS_PER_S 1000u

esp_err_t uart_bridge_start(void);

#endif

#ifndef GW_H2_CONFIG_H
#define GW_H2_CONFIG_H

#include "shared_config.h"

#define FW_NAME "gw_h2"

#define UART_TX_GPIO 10
#define UART_RX_GPIO 11

#define BRIDGE_TASK_MS 50u
#define ZB_EVT_QUEUE_LEN 32

#define NC_MAX 4
#define EP_MASK_ALL 0x3Fu
#define UNKNOWN_IEEE_HOLD_MS 5000u

#define PERMIT_JOIN_MIN_S 1
#define PERMIT_JOIN_MAX_S 254

#define ZB_TASK_STACK 8192
#define ZB_TASK_PRIO 5
#define ZB_COORD_RESET_DELAY_MS 1000u

#endif

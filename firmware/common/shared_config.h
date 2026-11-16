#ifndef FIRE_COMMON_SHARED_CONFIG_H
#define FIRE_COMMON_SHARED_CONFIG_H

/* Nhom co the ghi de bang co build (-D...). */
#ifndef ZB_CHANNEL
#define ZB_CHANNEL 25
#endif
#ifndef ZB_PAN_ID
#define ZB_PAN_ID 0x1A62
#endif
#ifndef COORD_EP
#define COORD_EP 1
#endif
#define ZCL_CENTI_SCALE 100.0f
#define COORD_SHORT_ADDR 0x0000u
#define ZB_INSTALL_CODE_POLICY false

/* Nhom bat bien theo hop dong GatewayPlan §4.2. */
#define EP_TEMP 10
#define EP_HUM 10
#define EP_MQ2 11
#define EP_FLAME 12
#define EP_ECO2 13
#define EP_TVOC 14
#define EP_FLAGS 15

/* Nhom co the ghi de bang co build (-D...). */
#ifndef REPORT_NORMAL_MS
#define REPORT_NORMAL_MS 10000u
#endif
#ifndef REPORT_PREALERT_MS
#define REPORT_PREALERT_MS 3000u
#endif
#define REPORT_MIN_GAP_MS 1000u

#ifndef HB_INTERVAL_MS
#define HB_INTERVAL_MS 2000u
#endif
#ifndef HB_TIMEOUT_MS
#define HB_TIMEOUT_MS 6000u
#endif

/* COALESCE_MS nam trong hop dong bao cao. */
#define COALESCE_MS 300u
#define JOIN_WINDOW_S 180u

#define NODE_OFFLINE_MIN_MS 30000u
#define NODE_OFFLINE_FACTOR 3u

#ifndef UART_BAUD
#define UART_BAUD 115200
#endif
#define UART_RX_BUF 1024
#define UART_TX_BUF 1024

#ifndef BENCH_MODE
#define BENCH_MODE 0
#endif
#ifndef RELEASE_BUILD
#define RELEASE_BUILD 0
#endif
#if BENCH_MODE && RELEASE_BUILD
#error "BENCH_MODE phai la 0 trong build release"
#endif

#endif

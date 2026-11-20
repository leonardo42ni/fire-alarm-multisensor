#ifndef ZB_NODE_CONFIG_H
#define ZB_NODE_CONFIG_H

#include "shared_config.h"

#define ZB_TASK_STACK 8192
#define ZB_TASK_PRIO 5
#define ZB_RETRY_MIN_MS 5000u
#define ZB_RETRY_MAX_MS 60000u
#define ZB_LOCK_TIMEOUT_MS 1000u
#define ZB_REJOIN_FAIL_THRESHOLD 3u
#define ZB_KEEP_ALIVE_MS REPORT_PREALERT_MS
#define ZB_NODE_FLAG_FLAME_DO (1u << 3)
#define ZB_NODE_FLAG_PREALERT (1u << 4)

#endif

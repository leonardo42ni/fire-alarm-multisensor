#ifndef TEST_PROTO_COMMON_H
#define TEST_PROTO_COMMON_H

#include <string.h>

#include "proto_ndjson.h"

#define IEEE "0011223344556677"

static inline proto_err_t dec(const char *line, proto_msg_t *m)
{
    return proto_dec(line, strlen(line), m);
}

#endif /* TEST_PROTO_COMMON_H */

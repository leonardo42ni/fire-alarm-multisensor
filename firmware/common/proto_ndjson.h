#ifndef PROTO_NDJSON_H
#define PROTO_NDJSON_H

/* NDJSON line codec for the GW-H2 <-> GW-HOST UART link.
 * Contract: docs/GatewayPlan.md section 5.1 / docs/DEV_CODING_PLAN.md section 5.1.
 * Pure C11, no ESP-IDF or dynamic allocation dependencies. */

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PROTO_MAX_LINE 256
#define PROTO_IEEE_LEN 17 /* 16 hex + NUL */
#define PROTO_FW_LEN 24
#define PROTO_OP_LEN 16
#define PROTO_TYPE_LEN 12
#define PROTO_HEX_BUF_LEN 16
#define PROTO_NUM_BUF_LEN 64
#define PROTO_JSON_MAX_DEPTH 16
#define PROTO_U8_MAX 255.0
#define PROTO_U16_MAX 65535.0
#define PROTO_U32_MAX 4294967295.0
#define PROTO_PAN_MAX 0xFFFFu
#define PROTO_RSSI_MIN (-32768.0)
#define PROTO_RSSI_MAX 32767.0

typedef enum {
    PROTO_OK = 0,
    PROTO_E_TOO_LONG = -1,
    PROTO_E_SYNTAX = -2,
    PROTO_E_TYPE = -3,
    PROTO_E_FIELD = -4,
    PROTO_E_BUF = -5
} proto_err_t;

typedef enum {
    M_NONE = 0,
    M_HELLO,
    M_HB,
    M_JOIN,
    M_LEAVE,
    M_RPT,
    M_CMD,
    M_ACK
} msg_type_t;

typedef enum {
    OP_NONE = 0,
    OP_PERMIT_JOIN,
    OP_RESET_NET,
    OP_PING
} cmd_op_t;

#define RPT_HAS_TEMP (1u << 0)
#define RPT_HAS_HUM (1u << 1)
#define RPT_HAS_ECO2 (1u << 2)
#define RPT_HAS_TVOC (1u << 3)
#define RPT_HAS_MQ2 (1u << 4)
#define RPT_HAS_FLAME (1u << 5)
#define RPT_HAS_FLAGS (1u << 6)

typedef struct {
    char ieee[PROTO_IEEE_LEN];
    uint16_t sa;
    uint32_t seq;
    float temp, hum, eco2, tvoc, mq2_mv, flame_mv;
    uint16_t flags;
    int16_t rssi; /* 0 = unknown (always emitted) */
    uint8_t lqi;  /* 0 = unknown (always emitted) */
    uint32_t valid;
} rpt_t;

typedef struct {
    msg_type_t type;
    union {
        struct {
            char fw[PROTO_FW_LEN];
            uint8_t ch;
            uint16_t pan;
        } hello;
        struct {
            uint32_t up;
            uint8_t nodes;
        } hb;
        struct {
            char ieee[PROTO_IEEE_LEN];
            uint16_t sa;
        } join, leave;
        rpt_t rpt;
        struct {
            cmd_op_t op;
            uint16_t sec;
        } cmd;
        struct {
            cmd_op_t op;
        } ack;
    } u;
} proto_msg_t;

/* Encoders: return bytes written (including '\n', excluding NUL) or a
 * negative PROTO_E_* value. On error buf[0] is set to NUL. NULL buf gives
 * PROTO_E_BUF; NULL fw/ieee/rpt gives PROTO_E_FIELD. */
int proto_enc_hello(char *buf, size_t cap, const char *fw, uint8_t ch, uint16_t pan);
int proto_enc_hb(char *buf, size_t cap, uint32_t up, uint8_t nodes);
int proto_enc_join(char *buf, size_t cap, const char *ieee, uint16_t sa);
int proto_enc_leave(char *buf, size_t cap, const char *ieee, uint16_t sa);
int proto_enc_rpt(char *buf, size_t cap, const rpt_t *rpt);
int proto_enc_cmd(char *buf, size_t cap, cmd_op_t op, uint16_t sec);
int proto_enc_ack(char *buf, size_t cap, cmd_op_t op);

/* Decoder. `len` may include optional trailing '\r'/'\n'. NULL line/out
 * gives PROTO_E_FIELD; out is zeroed (type M_NONE) on any error. */
proto_err_t proto_dec(const char *line, size_t len, proto_msg_t *out);

/* Flat JSON helpers (depth 1 keys only). flat_get_str rejects escaped
 * strings. flat_get_num assumes input already passed proto_json_valid_object
 * (strtod alone would also accept 0x-prefixed hex). */
int flat_find(const char *s, size_t n, const char *key, const char **vs, size_t *vl);
int flat_get_num(const char *s, size_t n, const char *key, double *out);
int flat_get_str(const char *s, size_t n, const char *key, char *out, size_t cap);

/* Line assembler. */
typedef void (*line_cb_t)(const char *line, size_t len, void *ctx);

typedef struct {
    char buf[PROTO_MAX_LINE];
    size_t len;
    bool overflow;
    uint32_t n_lines, n_overflow;
} line_asm_t;

void line_asm_init(line_asm_t *la);
size_t line_asm_feed(line_asm_t *la, const uint8_t *d, size_t n, line_cb_t cb, void *ctx);

#ifdef __cplusplus
}
#endif

#endif /* PROTO_NDJSON_H */

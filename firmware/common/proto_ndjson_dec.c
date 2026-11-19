#include "proto_ndjson.h"
#include "proto_ndjson_internal.h"

#include <float.h>
#include <string.h>

static proto_err_t read_str_req(const char *s, size_t n, const char *key,
                                char *out, size_t cap)
{
    const char *vs = NULL;
    size_t vl = 0;
    if (!flat_find(s, n, key, &vs, &vl)) return PROTO_E_FIELD;
    if (vl < 2 || vs[0] != '"' || vs[vl - 1] != '"') return PROTO_E_TYPE;
    if (!flat_get_str(s, n, key, out, cap)) return PROTO_E_TYPE;
    return PROTO_OK;
}

static proto_err_t read_num_opt(const char *s, size_t n, const char *key,
                                double *out, int *found)
{
    const char *vs = NULL;
    size_t vl = 0;
    *found = 0;
    if (!flat_find(s, n, key, &vs, &vl)) return PROTO_OK;
    *found = 1;
    if (!flat_get_num(s, n, key, out)) return PROTO_E_TYPE;
    return PROTO_OK;
}

static proto_err_t read_num_req(const char *s, size_t n, const char *key, double *out)
{
    const char *vs = NULL;
    size_t vl = 0;
    if (!flat_find(s, n, key, &vs, &vl)) return PROTO_E_FIELD;
    if (!flat_get_num(s, n, key, out)) return PROTO_E_TYPE;
    return PROTO_OK;
}

static int d_to_u64(double d, double max, uint64_t *out)
{
    uint64_t v;
    if (!(d >= 0.0) || d > max) return 0;
    v = (uint64_t)d;
    if ((double)v != d) return 0;
    *out = v;
    return 1;
}

static proto_err_t read_u64_opt(const char *s, size_t n, const char *key,
                                double max, uint64_t *out, int *found)
{
    double d = 0.0;
    proto_err_t e = read_num_opt(s, n, key, &d, found);
    if (e != PROTO_OK) return e;
    if (*found && !d_to_u64(d, max, out)) return PROTO_E_TYPE;
    return PROTO_OK;
}

static proto_err_t read_u64_req(const char *s, size_t n, const char *key,
                                double max, uint64_t *out)
{
    double d = 0.0;
    proto_err_t e = read_num_req(s, n, key, &d);
    if (e != PROTO_OK) return e;
    if (!d_to_u64(d, max, out)) return PROTO_E_TYPE;
    return PROTO_OK;
}

static int parse_hex(const char *txt, uint32_t max, uint32_t *out)
{
    const char *p = txt;
    uint32_t v = 0;
    size_t digits = 0;

    if (p == NULL) return 0;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) p += 2;
    for (; *p != '\0'; p++) {
        int h = proto_hex_val(*p);
        if (h < 0) return 0;
        if ((uint32_t)h > max || v > (max - (uint32_t)h) / 16u) return 0;
        v = (v << 4) | (uint32_t)h;
        digits++;
    }
    if (digits == 0) return 0;
    *out = v;
    return 1;
}

static proto_err_t read_hex_opt(const char *s, size_t n, const char *key,
                                uint32_t max, uint32_t *out, int *found)
{
    char tmp[PROTO_HEX_BUF_LEN];
    const char *vs = NULL;
    size_t vl = 0;
    *found = 0;
    if (!flat_find(s, n, key, &vs, &vl)) return PROTO_OK;
    if (!flat_get_str(s, n, key, tmp, sizeof(tmp))) return PROTO_E_TYPE;
    if (!parse_hex(tmp, max, out)) return PROTO_E_TYPE;
    *found = 1;
    return PROTO_OK;
}

static proto_err_t read_hex_req(const char *s, size_t n, const char *key,
                                uint32_t max, uint32_t *out)
{
    char tmp[PROTO_HEX_BUF_LEN];
    const char *vs = NULL;
    size_t vl = 0;
    if (!flat_find(s, n, key, &vs, &vl)) return PROTO_E_FIELD;
    if (!flat_get_str(s, n, key, tmp, sizeof(tmp))) return PROTO_E_TYPE;
    if (!parse_hex(tmp, max, out)) return PROTO_E_TYPE;
    return PROTO_OK;
}

static msg_type_t type_from_name(const char *t)
{
    if (strcmp(t, "hello") == 0) return M_HELLO;
    if (strcmp(t, "hb") == 0) return M_HB;
    if (strcmp(t, "join") == 0) return M_JOIN;
    if (strcmp(t, "leave") == 0) return M_LEAVE;
    if (strcmp(t, "rpt") == 0) return M_RPT;
    if (strcmp(t, "cmd") == 0) return M_CMD;
    if (strcmp(t, "ack") == 0) return M_ACK;
    return M_NONE;
}

static cmd_op_t op_from_name(const char *op)
{
    if (strcmp(op, "permit_join") == 0) return OP_PERMIT_JOIN;
    if (strcmp(op, "reset_net") == 0) return OP_RESET_NET;
    if (strcmp(op, "ping") == 0) return OP_PING;
    return OP_NONE;
}

#define GET_OPT_NUM(field, bit)                                                \
    do {                                                                       \
        e = read_num_opt(line, len, #field, &d, &found);                       \
        if (e != PROTO_OK) return e;                                           \
        if (found) {                                                           \
            if (d > (double)FLT_MAX || d < -(double)FLT_MAX) return PROTO_E_TYPE; \
            r->field = (float)d;                                               \
            r->valid |= (bit);                                                 \
        }                                                                      \
    } while (0)

static proto_err_t dec_rpt(const char *line, size_t len, rpt_t *r)
{
    double d;
    uint64_t u;
    int found;
    proto_err_t e;

    e = read_str_req(line, len, "ieee", r->ieee, sizeof(r->ieee));
    if (e != PROTO_OK) return e;
    if (!proto_ieee_ok(r->ieee)) return PROTO_E_FIELD;
    e = read_u64_req(line, len, "seq", PROTO_U32_MAX, &u);
    if (e != PROTO_OK) return e;
    r->seq = (uint32_t)u;
    GET_OPT_NUM(temp, RPT_HAS_TEMP);
    GET_OPT_NUM(hum, RPT_HAS_HUM);
    GET_OPT_NUM(eco2, RPT_HAS_ECO2);
    GET_OPT_NUM(tvoc, RPT_HAS_TVOC);
    GET_OPT_NUM(mq2_mv, RPT_HAS_MQ2);
    GET_OPT_NUM(flame_mv, RPT_HAS_FLAME);
    e = read_u64_opt(line, len, "flags", PROTO_U16_MAX, &u, &found);
    if (e != PROTO_OK) return e;
    if (found) { r->flags = (uint16_t)u; r->valid |= RPT_HAS_FLAGS; }
    e = read_num_opt(line, len, "rssi", &d, &found);
    if (e != PROTO_OK) return e;
    if (found) {
        if (d < PROTO_RSSI_MIN || d > PROTO_RSSI_MAX || (double)(int)d != d) {
            return PROTO_E_TYPE;
        }
        r->rssi = (int16_t)d;
    }
    e = read_u64_opt(line, len, "lqi", PROTO_U8_MAX, &u, &found);
    if (e != PROTO_OK) return e;
    if (found) r->lqi = (uint8_t)u;
    return PROTO_OK;
}

static proto_err_t dec_hello(const char *line, size_t len, proto_msg_t *out)
{
    uint64_t u;
    uint32_t pan = 0;
    proto_err_t e;

    e = read_str_req(line, len, "fw", out->u.hello.fw, sizeof(out->u.hello.fw));
    if (e != PROTO_OK) return e;
    e = read_u64_req(line, len, "ch", PROTO_U8_MAX, &u);
    if (e != PROTO_OK) return e;
    out->u.hello.ch = (uint8_t)u;
    e = read_hex_req(line, len, "pan", PROTO_PAN_MAX, &pan);
    if (e != PROTO_OK) return e;
    out->u.hello.pan = (uint16_t)pan;
    return PROTO_OK;
}

static proto_err_t dec_hb(const char *line, size_t len, proto_msg_t *out)
{
    uint64_t u;
    proto_err_t e;

    e = read_u64_req(line, len, "up", PROTO_U32_MAX, &u);
    if (e != PROTO_OK) return e;
    out->u.hb.up = (uint32_t)u;
    e = read_u64_req(line, len, "nodes", PROTO_U8_MAX, &u);
    if (e != PROTO_OK) return e;
    out->u.hb.nodes = (uint8_t)u;
    return PROTO_OK;
}

static proto_err_t dec_join(const char *line, size_t len, proto_msg_t *out)
{
    uint32_t v = 0;
    int found = 0;
    proto_err_t e;

    e = read_str_req(line, len, "ieee", out->u.join.ieee, sizeof(out->u.join.ieee));
    if (e != PROTO_OK) return e;
    if (!proto_ieee_ok(out->u.join.ieee)) return PROTO_E_FIELD;
    e = read_hex_opt(line, len, "sa", PROTO_PAN_MAX, &v, &found);
    if (e != PROTO_OK) return e;
    out->u.join.sa = (uint16_t)v;
    return PROTO_OK;
}

static proto_err_t dec_leave(const char *line, size_t len, proto_msg_t *out)
{
    proto_err_t e = read_str_req(line, len, "ieee", out->u.leave.ieee,
                                 sizeof(out->u.leave.ieee));
    if (e != PROTO_OK) return e;
    if (!proto_ieee_ok(out->u.leave.ieee)) return PROTO_E_FIELD;
    return PROTO_OK;
}

static proto_err_t dec_op(const char *line, size_t len, cmd_op_t *op)
{
    char opbuf[PROTO_OP_LEN];
    proto_err_t e = read_str_req(line, len, "op", opbuf, sizeof(opbuf));
    if (e != PROTO_OK) return e;
    *op = op_from_name(opbuf);
    if (*op == OP_NONE) return PROTO_E_TYPE;
    return PROTO_OK;
}

static proto_err_t dec_cmd(const char *line, size_t len, proto_msg_t *out)
{
    uint64_t u;
    int found = 0;
    proto_err_t e = dec_op(line, len, &out->u.cmd.op);
    if (e != PROTO_OK) return e;
    if (out->u.cmd.op != OP_PERMIT_JOIN) return PROTO_OK;
    e = read_u64_opt(line, len, "sec", PROTO_U16_MAX, &u, &found);
    if (e != PROTO_OK) return e;
    if (found) out->u.cmd.sec = (uint16_t)u;
    return PROTO_OK;
}

proto_err_t proto_dec(const char *line, size_t len, proto_msg_t *out)
{
    char tbuf[PROTO_TYPE_LEN];
    proto_err_t e;
    msg_type_t type;

    if (line == NULL || out == NULL) return PROTO_E_FIELD;
    memset(out, 0, sizeof(*out));
    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) len--;
    if (len == 0) return PROTO_E_SYNTAX;
    if (len >= PROTO_MAX_LINE) return PROTO_E_TOO_LONG;
    if (line[0] != '{' || line[len - 1] != '}') return PROTO_E_SYNTAX;
    if (!proto_json_valid_object(line, len)) return PROTO_E_SYNTAX;
    e = read_str_req(line, len, "t", tbuf, sizeof(tbuf));
    if (e != PROTO_OK) return e;
    type = type_from_name(tbuf);
    if (type == M_NONE) return PROTO_E_TYPE;
    out->type = type;
    switch (type) {
    case M_HELLO: e = dec_hello(line, len, out); break;
    case M_HB: e = dec_hb(line, len, out); break;
    case M_JOIN: e = dec_join(line, len, out); break;
    case M_LEAVE: e = dec_leave(line, len, out); break;
    case M_RPT: e = dec_rpt(line, len, &out->u.rpt); break;
    case M_CMD: e = dec_cmd(line, len, out); break;
    case M_ACK: e = dec_op(line, len, &out->u.ack.op); break;
    default: e = PROTO_E_TYPE; break;
    }
    if (e != PROTO_OK) {
        memset(out, 0, sizeof(*out));
    }
    return e;
}

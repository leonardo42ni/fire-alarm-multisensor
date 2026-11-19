#include "proto_ndjson.h"
#include "proto_ndjson_internal.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

static int plain_str_ok(const char *s, size_t max)
{
    size_t i;

    if (s == NULL) return 0;
    for (i = 0; s[i] != '\0'; i++) {
        unsigned char c = (unsigned char)s[i];
        if (i + 1 >= max) return 0;
        if (c < 0x20 || c == '"' || c == '\\') return 0;
    }
    return 1;
}

static const char *op_name(cmd_op_t op)
{
    switch (op) {
    case OP_PERMIT_JOIN: return "permit_join";
    case OP_RESET_NET: return "reset_net";
    case OP_PING: return "ping";
    default: return NULL;
    }
}

static int enc_fail(char *buf, size_t cap, int err)
{
    if (buf != NULL && cap > 0) buf[0] = '\0';
    return err;
}

static int enc_ready(char *buf, size_t cap)
{
    if (buf == NULL || cap == 0) return 0;
    buf[0] = '\0';
    return 1;
}

#define ENC_CHK(expr)                                                          \
    do {                                                                       \
        int _n = (expr);                                                       \
        if (_n < 0 || (size_t)_n >= cap - off) return enc_fail(buf, cap, PROTO_E_BUF); \
        off += (size_t)_n;                                                     \
    } while (0)

int proto_enc_hello(char *buf, size_t cap, const char *fw, uint8_t ch, uint16_t pan)
{
    int n;

    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    if (!plain_str_ok(fw, PROTO_FW_LEN)) return enc_fail(buf, cap, PROTO_E_FIELD);
    n = snprintf(buf, cap, "{\"t\":\"hello\",\"fw\":\"%s\",\"ch\":%u,\"pan\":\"0x%04X\"}\n",
                 fw, (unsigned)ch, (unsigned)pan);
    if (n < 0 || (size_t)n >= cap) return enc_fail(buf, cap, PROTO_E_BUF);
    return n;
}

int proto_enc_hb(char *buf, size_t cap, uint32_t up, uint8_t nodes)
{
    int n;

    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    n = snprintf(buf, cap, "{\"t\":\"hb\",\"up\":%lu,\"nodes\":%u}\n",
                 (unsigned long)up, (unsigned)nodes);
    if (n < 0 || (size_t)n >= cap) return enc_fail(buf, cap, PROTO_E_BUF);
    return n;
}

int proto_enc_join(char *buf, size_t cap, const char *ieee, uint16_t sa)
{
    size_t off = 0;

    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    if (!proto_ieee_ok(ieee)) return enc_fail(buf, cap, PROTO_E_FIELD);
    ENC_CHK(snprintf(buf + off, cap - off, "{\"t\":\"join\",\"ieee\":\"%s\",\"sa\":\"0x%04X\"}\n",
                     ieee, (unsigned)sa));
    return (int)off;
}

int proto_enc_leave(char *buf, size_t cap, const char *ieee, uint16_t sa)
{
    size_t off = 0;

    (void)sa;
    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    if (!proto_ieee_ok(ieee)) return enc_fail(buf, cap, PROTO_E_FIELD);
    ENC_CHK(snprintf(buf + off, cap - off, "{\"t\":\"leave\",\"ieee\":\"%s\"}\n", ieee));
    return (int)off;
}

int proto_enc_rpt(char *buf, size_t cap, const rpt_t *rpt)
{
    size_t off = 0;
    uint32_t valid;

    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    if (rpt == NULL || !proto_ieee_ok(rpt->ieee)) return enc_fail(buf, cap, PROTO_E_FIELD);
    valid = rpt->valid;
    ENC_CHK(snprintf(buf + off, cap - off, "{\"t\":\"rpt\",\"ieee\":\"%s\",\"seq\":%lu",
                     rpt->ieee, (unsigned long)rpt->seq));
    if ((valid & RPT_HAS_TEMP) != 0 && isfinite(rpt->temp)) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"temp\":%.2f", (double)rpt->temp));
    }
    if ((valid & RPT_HAS_HUM) != 0 && isfinite(rpt->hum)) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"hum\":%.2f", (double)rpt->hum));
    }
    if ((valid & RPT_HAS_ECO2) != 0 && isfinite(rpt->eco2)) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"eco2\":%.0f", (double)rpt->eco2));
    }
    if ((valid & RPT_HAS_TVOC) != 0 && isfinite(rpt->tvoc)) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"tvoc\":%.0f", (double)rpt->tvoc));
    }
    if ((valid & RPT_HAS_MQ2) != 0 && isfinite(rpt->mq2_mv)) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"mq2_mv\":%.1f", (double)rpt->mq2_mv));
    }
    if ((valid & RPT_HAS_FLAME) != 0 && isfinite(rpt->flame_mv)) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"flame_mv\":%.1f", (double)rpt->flame_mv));
    }
    if ((valid & RPT_HAS_FLAGS) != 0) {
        ENC_CHK(snprintf(buf + off, cap - off, ",\"flags\":%u", (unsigned)rpt->flags));
    }
    ENC_CHK(snprintf(buf + off, cap - off, ",\"rssi\":%d,\"lqi\":%u}\n",
                     (int)rpt->rssi, (unsigned)rpt->lqi));
    return (int)off;
}

int proto_enc_cmd(char *buf, size_t cap, cmd_op_t op, uint16_t sec)
{
    size_t off = 0;
    const char *name = op_name(op);

    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    if (name == NULL) return enc_fail(buf, cap, PROTO_E_FIELD);
    if (op == OP_PERMIT_JOIN) {
        ENC_CHK(snprintf(buf + off, cap - off, "{\"t\":\"cmd\",\"op\":\"%s\",\"sec\":%u}\n",
                         name, (unsigned)sec));
    } else {
        ENC_CHK(snprintf(buf + off, cap - off, "{\"t\":\"cmd\",\"op\":\"%s\"}\n", name));
    }
    return (int)off;
}

int proto_enc_ack(char *buf, size_t cap, cmd_op_t op)
{
    size_t off = 0;
    const char *name = op_name(op);

    if (!enc_ready(buf, cap)) return PROTO_E_BUF;
    if (name == NULL) return enc_fail(buf, cap, PROTO_E_FIELD);
    ENC_CHK(snprintf(buf + off, cap - off, "{\"t\":\"ack\",\"op\":\"%s\"}\n", name));
    return (int)off;
}

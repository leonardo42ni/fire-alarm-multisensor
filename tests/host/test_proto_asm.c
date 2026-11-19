#include "test_util.h"

#include <string.h>

#include "proto_ndjson.h"
#include "test_proto_common.h"

typedef struct {
    int count;
    size_t len[4];
    char line[4][PROTO_MAX_LINE];
} asm_ctx_t;

static void asm_cb(const char *line, size_t len, void *ctx)
{
    asm_ctx_t *c = (asm_ctx_t *)ctx;

    if (c->count < 4 && len < PROTO_MAX_LINE) {
        memcpy(c->line[c->count], line, len);
        c->line[c->count][len] = '\0';
        c->len[c->count] = len;
    }
    c->count++;
}

static int test_asm_no_newline_no_callback(void)
{
    line_asm_t la;
    asm_ctx_t ctx;
    const char *txt = "{\"t\":\"hb\",\"up\":1,\"nodes\":1}";

    memset(&ctx, 0, sizeof(ctx));
    line_asm_init(&la);
    TEST_ASSERT_EQUAL_INT(0, (int)line_asm_feed(&la, (const uint8_t *)txt, strlen(txt), asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(0, ctx.count);
    TEST_ASSERT_EQUAL_INT(0, la.n_lines);
    return 0;
}

static int test_asm_multiple_lines(void)
{
    line_asm_t la;
    asm_ctx_t ctx;
    const char *txt = "one\n\ntwo\r\n";

    memset(&ctx, 0, sizeof(ctx));
    line_asm_init(&la);
    TEST_ASSERT_EQUAL_INT(2, (int)line_asm_feed(&la, (const uint8_t *)txt, strlen(txt), asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(2, ctx.count);
    TEST_ASSERT_EQUAL_INT(3, ctx.len[0]);
    TEST_ASSERT_STR_EQUAL("one", ctx.line[0]);
    TEST_ASSERT_EQUAL_INT(3, ctx.len[1]);
    TEST_ASSERT_STR_EQUAL("two", ctx.line[1]);
    TEST_ASSERT_EQUAL_INT(2, la.n_lines);
    TEST_ASSERT_EQUAL_INT(0, la.n_overflow);
    return 0;
}

static int test_asm_overflow_discards_line(void)
{
    line_asm_t la;
    asm_ctx_t ctx;
    char big[400];
    const char *valid = "{\"t\":\"hb\",\"up\":1,\"nodes\":1}\n";

    memset(&ctx, 0, sizeof(ctx));
    memset(big, 'A', sizeof(big));
    line_asm_init(&la);
    TEST_ASSERT_EQUAL_INT(0, (int)line_asm_feed(&la, (const uint8_t *)big, sizeof(big), asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(1, la.n_overflow);
    TEST_ASSERT_EQUAL_INT(0, (int)line_asm_feed(&la, (const uint8_t *)"\n", 1, asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(0, ctx.count);
    TEST_ASSERT_EQUAL_INT(1, (int)line_asm_feed(&la, (const uint8_t *)valid, strlen(valid), asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(1, ctx.count);
    TEST_ASSERT_EQUAL_INT(1, la.n_overflow);
    TEST_ASSERT_EQUAL_INT(1, la.n_lines);
    return 0;
}

static int test_asm_boundary_255(void)
{
    line_asm_t la;
    asm_ctx_t ctx;
    char line[PROTO_MAX_LINE + 1];
    char nl = '\n';

    memset(&ctx, 0, sizeof(ctx));
    memset(line, 'x', 255);
    line[255] = '\n';
    line_asm_init(&la);
    TEST_ASSERT_EQUAL_INT(1, (int)line_asm_feed(&la, (const uint8_t *)line, 256, asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(255, ctx.len[0]);

    memset(&ctx, 0, sizeof(ctx));
    memset(line, 'x', PROTO_MAX_LINE);
    line[PROTO_MAX_LINE] = '\n';
    line_asm_init(&la);
    TEST_ASSERT_EQUAL_INT(0, (int)line_asm_feed(&la, (const uint8_t *)line, sizeof(line), asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(1, la.n_overflow);
    TEST_ASSERT_EQUAL_INT(0, (int)line_asm_feed(&la, (const uint8_t *)&nl, 1, asm_cb, &ctx));
    return 0;
}

static int test_asm_boundary_255_crlf(void)
{
    line_asm_t la;
    asm_ctx_t ctx;
    char line[PROTO_MAX_LINE + 1];

    memset(&ctx, 0, sizeof(ctx));
    memset(line, 'x', PROTO_MAX_LINE - 1);
    line[PROTO_MAX_LINE - 1] = '\r';
    line[PROTO_MAX_LINE] = '\n';
    line_asm_init(&la);
    TEST_ASSERT_EQUAL_INT(1, (int)line_asm_feed(&la, (const uint8_t *)line, sizeof(line), asm_cb, &ctx));
    TEST_ASSERT_EQUAL_INT(1, ctx.count);
    TEST_ASSERT_EQUAL_INT(255, ctx.len[0]);
    TEST_ASSERT_EQUAL_INT(0, la.n_overflow);
    return 0;
}

static proto_err_t rt_decode(const char *enc, size_t n, proto_msg_t *m)
{
    line_asm_t la;
    asm_ctx_t ctx;

    memset(&ctx, 0, sizeof(ctx));
    line_asm_init(&la);
    if (line_asm_feed(&la, (const uint8_t *)enc, n, asm_cb, &ctx) != 1) return PROTO_E_SYNTAX;
    return proto_dec(ctx.line[0], ctx.len[0], m);
}

static int test_roundtrip_all(void)
{
    char buf[256];
    proto_msg_t m;
    rpt_t r;
    int n;

    n = proto_enc_hello(buf, sizeof(buf), "gwh2-0.1.0", 25, 0x1A62);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip hello");
    TEST_ASSERT_EQUAL_INT(M_HELLO, m.type);

    n = proto_enc_hb(buf, sizeof(buf), 9, 2);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip hb");
    TEST_ASSERT_EQUAL_INT(M_HB, m.type);

    n = proto_enc_join(buf, sizeof(buf), IEEE, 0x1234);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip join");
    TEST_ASSERT_EQUAL_INT(M_JOIN, m.type);

    n = proto_enc_leave(buf, sizeof(buf), IEEE, 0x1234);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip leave");
    TEST_ASSERT_EQUAL_INT(M_LEAVE, m.type);

    memset(&r, 0, sizeof(r));
    memcpy(r.ieee, IEEE, sizeof(IEEE));
    r.seq = 5;
    r.temp = 20.0f;
    r.valid = RPT_HAS_TEMP;
    n = proto_enc_rpt(buf, sizeof(buf), &r);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip rpt");
    TEST_ASSERT_EQUAL_INT(M_RPT, m.type);

    n = proto_enc_cmd(buf, sizeof(buf), OP_PERMIT_JOIN, 30);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip cmd");
    TEST_ASSERT_EQUAL_INT(M_CMD, m.type);

    n = proto_enc_ack(buf, sizeof(buf), OP_PING);
    TEST_ASSERT(n > 0 && rt_decode(buf, (size_t)n, &m) == PROTO_OK, "roundtrip ack");
    TEST_ASSERT_EQUAL_INT(M_ACK, m.type);
    return 0;
}

int main(void)
{
    test_run("asm_no_newline_no_callback", test_asm_no_newline_no_callback);
    test_run("asm_multiple_lines", test_asm_multiple_lines);
    test_run("asm_overflow_discards_line", test_asm_overflow_discards_line);
    test_run("asm_boundary_255", test_asm_boundary_255);
    test_run("asm_boundary_255_crlf", test_asm_boundary_255_crlf);
    test_run("roundtrip_all", test_roundtrip_all);
    return test_summary();
}

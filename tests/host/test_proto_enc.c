#include "test_util.h"

#include <math.h>
#include <string.h>

#include "proto_ndjson.h"
#include "test_proto_common.h"

static int test_enc_hello(void)
{
    char buf[128];
    const char *want = "{\"t\":\"hello\",\"fw\":\"gwh2-0.1.0\",\"ch\":25,\"pan\":\"0x1A62\"}\n";
    int n = proto_enc_hello(buf, sizeof(buf), "gwh2-0.1.0", 25, 0x1A62);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_hb(void)
{
    char buf[64];
    const char *want = "{\"t\":\"hb\",\"up\":123,\"nodes\":1}\n";
    int n = proto_enc_hb(buf, sizeof(buf), 123, 1);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_join(void)
{
    char buf[96];
    const char *want = "{\"t\":\"join\",\"ieee\":\"" IEEE "\",\"sa\":\"0x1234\"}\n";
    int n = proto_enc_join(buf, sizeof(buf), IEEE, 0x1234);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    TEST_ASSERT(proto_enc_join(buf, sizeof(buf), "00112233445566AA", 1) > 0, "uppercase IEEE");
    return 0;
}

static int test_enc_leave(void)
{
    char buf[96];
    const char *want = "{\"t\":\"leave\",\"ieee\":\"" IEEE "\"}\n";
    int n = proto_enc_leave(buf, sizeof(buf), IEEE, 0x9999);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_cmd_permit(void)
{
    char buf[96];
    const char *want = "{\"t\":\"cmd\",\"op\":\"permit_join\",\"sec\":180}\n";
    int n = proto_enc_cmd(buf, sizeof(buf), OP_PERMIT_JOIN, 180);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_cmd_no_sec(void)
{
    char buf[64];
    const char *want = "{\"t\":\"cmd\",\"op\":\"reset_net\"}\n";
    int n = proto_enc_cmd(buf, sizeof(buf), OP_RESET_NET, 0);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_ack(void)
{
    char buf[64];
    const char *want = "{\"t\":\"ack\",\"op\":\"ping\"}\n";
    int n = proto_enc_ack(buf, sizeof(buf), OP_PING);

    TEST_ASSERT_EQUAL_INT((int)strlen(want), n);
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_rpt_full(void)
{
    char buf[256];
    const char *want =
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":42,\"temp\":28.40,\"hum\":61.20,"
        "\"eco2\":410,\"tvoc\":5,\"mq2_mv\":1234.0,\"flame_mv\":4980.0,\"flags\":0,"
        "\"rssi\":-60,\"lqi\":200}\n";
    rpt_t r;

    memset(&r, 0, sizeof(r));
    memcpy(r.ieee, IEEE, sizeof(IEEE));
    r.seq = 42;
    r.temp = 28.4f;
    r.hum = 61.2f;
    r.eco2 = 410.0f;
    r.tvoc = 5.0f;
    r.mq2_mv = 1234.0f;
    r.flame_mv = 4980.0f;
    r.flags = 0;
    r.rssi = -60;
    r.lqi = 200;
    r.valid = RPT_HAS_TEMP | RPT_HAS_HUM | RPT_HAS_ECO2 | RPT_HAS_TVOC |
              RPT_HAS_MQ2 | RPT_HAS_FLAME | RPT_HAS_FLAGS;
    TEST_ASSERT_EQUAL_INT((int)strlen(want), proto_enc_rpt(buf, sizeof(buf), &r));
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_rpt_partial(void)
{
    char buf[160];
    const char *want = "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,"
                       "\"temp\":28.40,\"hum\":61.20,\"rssi\":0,\"lqi\":0}\n";
    rpt_t r;

    memset(&r, 0, sizeof(r));
    memcpy(r.ieee, IEEE, sizeof(IEEE));
    r.seq = 1;
    r.temp = 28.4f;
    r.hum = 61.2f;
    r.valid = RPT_HAS_TEMP | RPT_HAS_HUM;
    TEST_ASSERT_EQUAL_INT((int)strlen(want), proto_enc_rpt(buf, sizeof(buf), &r));
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_rpt_skips_nan_inf(void)
{
    char buf[160];
    const char *want = "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":2,"
                       "\"temp\":30.00,\"rssi\":0,\"lqi\":0}\n";
    rpt_t r;

    memset(&r, 0, sizeof(r));
    memcpy(r.ieee, IEEE, sizeof(IEEE));
    r.seq = 2;
    r.temp = 30.0f;
    r.hum = NAN;
    r.eco2 = INFINITY;
    r.valid = RPT_HAS_TEMP | RPT_HAS_HUM | RPT_HAS_ECO2;
    TEST_ASSERT_EQUAL_INT((int)strlen(want), proto_enc_rpt(buf, sizeof(buf), &r));
    TEST_ASSERT_STR_EQUAL(want, buf);
    return 0;
}

static int test_enc_rpt_ieee_no_nul(void)
{
    char buf[256];
    rpt_t r;

    memset(&r, 0, sizeof(r));
    memcpy(r.ieee, "00112233445566778", PROTO_IEEE_LEN);
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_rpt(buf, sizeof(buf), &r));
    return 0;
}

static int test_enc_buffer_too_small(void)
{
    char buf[8];

    buf[0] = 'x';
    TEST_ASSERT_EQUAL_INT(PROTO_E_BUF, proto_enc_hb(buf, sizeof(buf), 1, 1));
    TEST_ASSERT_EQUAL_INT('\0', buf[0]);
    TEST_ASSERT_EQUAL_INT(PROTO_E_BUF, proto_enc_hello(buf, 0, "gwh2-0.1.0", 25, 1));
    return 0;
}

static int test_enc_invalid_fields(void)
{
    char buf[128];
    char fw23[PROTO_FW_LEN];
    char fw24[PROTO_FW_LEN + 1];

    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_join(buf, sizeof(buf), "123", 1));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_join(buf, sizeof(buf), "001122334455667Z", 1));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_leave(buf, sizeof(buf), NULL, 1));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_hello(buf, sizeof(buf), "bad\"fw", 25, 1));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_cmd(buf, sizeof(buf), OP_NONE, 0));
    memset(fw23, 'a', sizeof(fw23) - 1);
    fw23[sizeof(fw23) - 1] = '\0';
    TEST_ASSERT(proto_enc_hello(buf, sizeof(buf), fw23, 25, 1) > 0, "fw 23 chars");
    memset(fw24, 'a', sizeof(fw24) - 1);
    fw24[sizeof(fw24) - 1] = '\0';
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, proto_enc_hello(buf, sizeof(buf), fw24, 25, 1));
    return 0;
}

int main(void)
{
    test_run("enc_hello", test_enc_hello);
    test_run("enc_hb", test_enc_hb);
    test_run("enc_join", test_enc_join);
    test_run("enc_leave", test_enc_leave);
    test_run("enc_cmd_permit", test_enc_cmd_permit);
    test_run("enc_cmd_no_sec", test_enc_cmd_no_sec);
    test_run("enc_ack", test_enc_ack);
    test_run("enc_rpt_full", test_enc_rpt_full);
    test_run("enc_rpt_partial", test_enc_rpt_partial);
    test_run("enc_rpt_skips_nan_inf", test_enc_rpt_skips_nan_inf);
    test_run("enc_rpt_ieee_no_nul", test_enc_rpt_ieee_no_nul);
    test_run("enc_buffer_too_small", test_enc_buffer_too_small);
    test_run("enc_invalid_fields", test_enc_invalid_fields);
    return test_summary();
}

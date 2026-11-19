#include "test_util.h"

#include <string.h>

#include "proto_ndjson.h"
#include "test_proto_common.h"

static int test_dec_too_long(void)
{
    proto_msg_t m;
    char line[300];

    memset(line, 'a', sizeof(line));
    line[0] = '{';
    line[sizeof(line) - 1] = '}';
    TEST_ASSERT_EQUAL_INT(PROTO_E_TOO_LONG, proto_dec(line, sizeof(line), &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, proto_dec(line, 0, &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, dec("\n", &m));
    TEST_ASSERT_EQUAL_INT(M_NONE, m.type);
    return 0;
}

static int test_dec_syntax_errors(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, dec("hello", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, dec("{\"t\":\"hb\"", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, dec("{\"t\":\"hb,\"up\":1}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, dec("{\"t\":\"hb\",\"up\":1,}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, dec("{\"t\":\"hb\"} trailing", &m));
    return 0;
}

static int test_dec_escape_nul_rejected(void)
{
    char line[] = "{\"t\":\"hb\",\"up\":1,\"x\":\"\\\0\"}";
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX, proto_dec(line, sizeof(line) - 1, &m));
    TEST_ASSERT_EQUAL_INT(M_NONE, m.type);
    return 0;
}

static int test_dec_missing_or_bad_t(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"up\":1}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE, dec("{\"t\":5}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE, dec("{\"t\":\"unknown\"}", &m));
    return 0;
}

static int test_dec_rpt_required_fields(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\"}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"rpt\",\"seq\":1}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"rpt\",\"ieee\":\"xyz\",\"seq\":1}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE,
                          dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":\"1\"}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE,
                          dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":-1}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE,
                          dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":4294967296}", &m));
    return 0;
}

static int test_dec_rpt_bad_optional_number(void)
{
    static const char *const lines[] = {
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"temp\":1e39}",
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"temp\":1e300}",
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"rssi\":40000}",
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"lqi\":256}",
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"flags\":65536}",
    };
    proto_msg_t m;
    size_t i;

    for (i = 0; i < sizeof(lines) / sizeof(lines[0]); i++) {
        TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE, dec(lines[i], &m));
        TEST_ASSERT_EQUAL_INT(M_NONE, m.type);
    }
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE,
                          dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"temp\":\"x\"}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_SYNTAX,
                          dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":1,\"temp\":NaN}", &m));
    return 0;
}

static int test_dec_cmd_errors(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"cmd\"}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE, dec("{\"t\":\"cmd\",\"op\":\"bogus\"}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE,
                          dec("{\"t\":\"cmd\",\"op\":\"permit_join\",\"sec\":70000}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"ack\"}", &m));
    return 0;
}

static int test_dec_hello_hb_errors(void)
{
    static const char *const bad_pan[] = {
        "{\"t\":\"hello\",\"fw\":\"x\",\"ch\":25,\"pan\":\"-0\"}",
        "{\"t\":\"hello\",\"fw\":\"x\",\"ch\":25,\"pan\":\" 1A62\"}",
        "{\"t\":\"hello\",\"fw\":\"x\",\"ch\":25,\"pan\":\"+1\"}",
        "{\"t\":\"hello\",\"fw\":\"x\",\"ch\":25,\"pan\":\"0x\"}",
        "{\"t\":\"hello\",\"fw\":\"x\",\"ch\":25,\"pan\":\"0x0x1A62\"}",
        "{\"t\":\"hello\",\"fw\":\"x\",\"ch\":25,\"pan\":\"0x10000\"}",
    };
    proto_msg_t m;
    size_t i;

    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"hello\",\"fw\":\"x\"}", &m));
    for (i = 0; i < sizeof(bad_pan) / sizeof(bad_pan[0]); i++) {
        TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE, dec(bad_pan[i], &m));
        TEST_ASSERT_EQUAL_INT(M_NONE, m.type);
    }
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE,
                          dec("{\"t\":\"hello\",\"fw\":\"a\\\"b\",\"ch\":25,\"pan\":\"0x1\"}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_FIELD, dec("{\"t\":\"hb\",\"up\":1}", &m));
    TEST_ASSERT_EQUAL_INT(PROTO_E_TYPE, dec("{\"t\":\"hb\",\"up\":1,\"nodes\":256}", &m));
    return 0;
}

int main(void)
{
    test_run("dec_too_long", test_dec_too_long);
    test_run("dec_syntax_errors", test_dec_syntax_errors);
    test_run("dec_escape_nul_rejected", test_dec_escape_nul_rejected);
    test_run("dec_missing_or_bad_t", test_dec_missing_or_bad_t);
    test_run("dec_rpt_required_fields", test_dec_rpt_required_fields);
    test_run("dec_rpt_bad_optional_number", test_dec_rpt_bad_optional_number);
    test_run("dec_cmd_errors", test_dec_cmd_errors);
    test_run("dec_hello_hb_errors", test_dec_hello_hb_errors);
    return test_summary();
}

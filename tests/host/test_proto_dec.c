#include "test_util.h"

#include <string.h>

#include "proto_ndjson.h"
#include "test_proto_common.h"

static int test_dec_hello(void)
{
    proto_msg_t m;
    const char *line = "{\"t\":\"hello\",\"fw\":\"gwh2-0.1.0\",\"ch\":25,\"pan\":\"0x1A62\"}";

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec(line, &m));
    TEST_ASSERT_EQUAL_INT(M_HELLO, m.type);
    TEST_ASSERT_STR_EQUAL("gwh2-0.1.0", m.u.hello.fw);
    TEST_ASSERT_EQUAL_INT(25, m.u.hello.ch);
    TEST_ASSERT_EQUAL_INT(0x1A62, m.u.hello.pan);
    return 0;
}

static int test_dec_hb_crlf(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"hb\",\"up\":123,\"nodes\":1}\r\n", &m));
    TEST_ASSERT_EQUAL_INT(M_HB, m.type);
    TEST_ASSERT_EQUAL_INT(123, m.u.hb.up);
    TEST_ASSERT_EQUAL_INT(1, m.u.hb.nodes);
    return 0;
}

static int test_dec_join_without_sa(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"join\",\"ieee\":\"" IEEE "\"}", &m));
    TEST_ASSERT_EQUAL_INT(M_JOIN, m.type);
    TEST_ASSERT_STR_EQUAL(IEEE, m.u.join.ieee);
    TEST_ASSERT_EQUAL_INT(0, m.u.join.sa);
    return 0;
}

static int test_dec_leave(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"leave\",\"ieee\":\"" IEEE "\"}\n", &m));
    TEST_ASSERT_EQUAL_INT(M_LEAVE, m.type);
    TEST_ASSERT_STR_EQUAL(IEEE, m.u.leave.ieee);
    return 0;
}

static int test_dec_rpt_full(void)
{
    proto_msg_t m;
    const char *line =
        "{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":42,\"temp\":28.4,\"hum\":61.2,"
        "\"eco2\":410,\"tvoc\":5,\"mq2_mv\":1234.0,\"flame_mv\":4980.0,\"flags\":3,"
        "\"rssi\":-60,\"lqi\":200}";

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec(line, &m));
    TEST_ASSERT_EQUAL_INT(M_RPT, m.type);
    TEST_ASSERT_STR_EQUAL(IEEE, m.u.rpt.ieee);
    TEST_ASSERT_EQUAL_INT(42, m.u.rpt.seq);
    TEST_ASSERT_EQUAL_INT(RPT_HAS_TEMP | RPT_HAS_HUM | RPT_HAS_ECO2 | RPT_HAS_TVOC |
                              RPT_HAS_MQ2 | RPT_HAS_FLAME | RPT_HAS_FLAGS,
                          m.u.rpt.valid);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 28.4, m.u.rpt.temp);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 61.2, m.u.rpt.hum);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 410.0, m.u.rpt.eco2);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1234.0, m.u.rpt.mq2_mv);
    TEST_ASSERT_EQUAL_INT(3, m.u.rpt.flags);
    TEST_ASSERT_EQUAL_INT(-60, m.u.rpt.rssi);
    TEST_ASSERT_EQUAL_INT(200, m.u.rpt.lqi);
    return 0;
}

static int test_dec_rpt_partial(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"rpt\",\"ieee\":\"" IEEE "\",\"seq\":7}", &m));
    TEST_ASSERT_EQUAL_INT(M_RPT, m.type);
    TEST_ASSERT_EQUAL_INT(7, m.u.rpt.seq);
    TEST_ASSERT_EQUAL_INT(0, m.u.rpt.valid);
    return 0;
}

static int test_dec_cmd(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"cmd\",\"op\":\"permit_join\",\"sec\":180}", &m));
    TEST_ASSERT_EQUAL_INT(M_CMD, m.type);
    TEST_ASSERT_EQUAL_INT(OP_PERMIT_JOIN, m.u.cmd.op);
    TEST_ASSERT_EQUAL_INT(180, m.u.cmd.sec);
    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"cmd\",\"op\":\"permit_join\",\"sec\":65535}", &m));
    TEST_ASSERT_EQUAL_INT(65535, m.u.cmd.sec);
    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"cmd\",\"op\":\"ping\"}", &m));
    TEST_ASSERT_EQUAL_INT(OP_PING, m.u.cmd.op);
    return 0;
}

static int test_dec_ack(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"ack\",\"op\":\"ping\"}\n", &m));
    TEST_ASSERT_EQUAL_INT(M_ACK, m.type);
    TEST_ASSERT_EQUAL_INT(OP_PING, m.u.ack.op);
    return 0;
}

static int test_dec_whitespace(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{ \"t\" : \"hb\" , \"up\" : 5 , \"nodes\" : 2 }\n", &m));
    TEST_ASSERT_EQUAL_INT(M_HB, m.type);
    TEST_ASSERT_EQUAL_INT(5, m.u.hb.up);
    TEST_ASSERT_EQUAL_INT(2, m.u.hb.nodes);
    return 0;
}

static int test_dec_reordered_keys(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK,
                          dec("{\"seq\":9,\"flags\":1,\"ieee\":\"" IEEE "\",\"t\":\"rpt\"}", &m));
    TEST_ASSERT_EQUAL_INT(M_RPT, m.type);
    TEST_ASSERT_EQUAL_INT(9, m.u.rpt.seq);
    TEST_ASSERT_EQUAL_INT(RPT_HAS_FLAGS, m.u.rpt.valid);
    return 0;
}

static int test_dec_unknown_key_ignored(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK,
                          dec("{\"t\":\"hb\",\"up\":1,\"nodes\":2,\"x\":{\"a\":[1,2]},\"y\":null}", &m));
    TEST_ASSERT_EQUAL_INT(M_HB, m.type);
    TEST_ASSERT_EQUAL_INT(1, m.u.hb.up);
    return 0;
}

static int test_dec_duplicate_key_first_wins(void)
{
    proto_msg_t m;

    TEST_ASSERT_EQUAL_INT(PROTO_OK, dec("{\"t\":\"hb\",\"up\":7,\"up\":9,\"nodes\":1}", &m));
    TEST_ASSERT_EQUAL_INT(7, m.u.hb.up);
    return 0;
}

static int test_flat_find_depth_and_strings(void)
{
    const char *line = "{\"x\":\"t:bogus\",\"t\":\"hb\",\"up\":3,\"nodes\":1}";
    const char *vs = NULL;
    size_t vl = 0;
    char out[8];

    TEST_ASSERT_EQUAL_INT(1, flat_find(line, strlen(line), "t", &vs, &vl));
    TEST_ASSERT_EQUAL_INT(1, flat_get_str(line, strlen(line), "t", out, sizeof(out)));
    TEST_ASSERT_STR_EQUAL("hb", out);
    TEST_ASSERT_EQUAL_INT(0, flat_find(line, strlen(line), "bogus", &vs, &vl));
    return 0;
}

static int test_flat_get_bad_types(void)
{
    const char *nums = "{\"n\":\"5\",\"m\":12x}";
    const char *strs = "{\"k\":\"abcdefgh\"}";
    double d = 0.0;
    char out[4];

    TEST_ASSERT_EQUAL_INT(0, flat_get_num(nums, strlen(nums), "n", &d));
    TEST_ASSERT_EQUAL_INT(0, flat_get_num(nums, strlen(nums), "m", &d));
    TEST_ASSERT_EQUAL_INT(0, flat_get_str(strs, strlen(strs), "k", out, sizeof(out)));
    return 0;
}

int main(void)
{
    test_run("dec_hello", test_dec_hello);
    test_run("dec_hb_crlf", test_dec_hb_crlf);
    test_run("dec_join_without_sa", test_dec_join_without_sa);
    test_run("dec_leave", test_dec_leave);
    test_run("dec_rpt_full", test_dec_rpt_full);
    test_run("dec_rpt_partial", test_dec_rpt_partial);
    test_run("dec_cmd", test_dec_cmd);
    test_run("dec_ack", test_dec_ack);
    test_run("dec_whitespace", test_dec_whitespace);
    test_run("dec_reordered_keys", test_dec_reordered_keys);
    test_run("dec_unknown_key_ignored", test_dec_unknown_key_ignored);
    test_run("dec_duplicate_key_first_wins", test_dec_duplicate_key_first_wins);
    test_run("flat_find_depth_and_strings", test_flat_find_depth_and_strings);
    test_run("flat_get_bad_types", test_flat_get_bad_types);
    return test_summary();
}

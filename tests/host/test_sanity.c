#include "test_util.h"

#include "error.h"
#include "version.h"

static int test_version_string(void)
{
    TEST_ASSERT_STR_EQUAL("0.1.0", FW_VERSION);
    return 0;
}

static int test_error_enum(void)
{
    TEST_ASSERT_EQUAL_INT(0, APP_OK);
    TEST_ASSERT_EQUAL_INT(-1, APP_ERR_INVALID_ARG);
    TEST_ASSERT_EQUAL_INT(-2, APP_ERR_INVALID_STATE);
    TEST_ASSERT_EQUAL_INT(-3, APP_ERR_NOT_FOUND);
    TEST_ASSERT_EQUAL_INT(-4, APP_ERR_NO_MEM);
    TEST_ASSERT_EQUAL_INT(-5, APP_ERR_TIMEOUT);
    TEST_ASSERT_EQUAL_INT(-6, APP_ERR_IO);
    TEST_ASSERT_EQUAL_INT(-7, APP_ERR_PARSE);
    TEST_ASSERT_EQUAL_INT(-8, APP_ERR_RANGE);
    return 0;
}

static int test_float_within(void)
{
    TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0, 1.0005);
    TEST_ASSERT_FLOAT_WITHIN(0.5, 3.0, 2.75);
    return 0;
}

int main(void)
{
    test_run("version_string", test_version_string);
    test_run("error_enum", test_error_enum);
    test_run("float_within", test_float_within);
    return test_summary();
}

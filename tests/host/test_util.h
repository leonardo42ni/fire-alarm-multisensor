#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define TEST_UTIL_UNUSED __attribute__((unused))
#else
#define TEST_UTIL_UNUSED
#endif

static TEST_UTIL_UNUSED int g_tests_run = 0;
static TEST_UTIL_UNUSED int g_tests_passed = 0;
static TEST_UTIL_UNUSED int g_tests_failed = 0;

typedef int (*test_fn_t)(void);

static TEST_UTIL_UNUSED int test_run(const char *name, test_fn_t fn)
{
    int rc;

    g_tests_run++;
    rc = fn();
    if (rc == 0) {
        g_tests_passed++;
        printf("[PASS] %s\n", name);
    } else {
        g_tests_failed++;
        printf("[FAIL] %s\n", name);
    }
    return rc;
}

static TEST_UTIL_UNUSED int test_summary(void)
{
    printf("[SUMMARY] run=%d passed=%d failed=%d\n",
           g_tests_run, g_tests_passed, g_tests_failed);
    return (g_tests_failed == 0) ? 0 : 1;
}

#define TEST_ASSERT(cond, msg)                                                \
    do {                                                                      \
        if (!(cond)) {                                                        \
            printf("[ASSERT] %s:%d: %s\n", __FILE__, __LINE__,               \
                   ((msg) != NULL) ? (msg) : "(null)");                       \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_EQUAL_INT(expected, actual)                               \
    do {                                                                      \
        long long _tu_int_expected = (long long)(expected);                   \
        long long _tu_int_actual = (long long)(actual);                       \
        if (_tu_int_expected != _tu_int_actual) {                             \
            printf("[ASSERT] %s:%d: expected %lld, got %lld\n",              \
                   __FILE__, __LINE__, _tu_int_expected, _tu_int_actual);     \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                     \
    do {                                                                      \
        double _tu_float_delta = fabs((double)(delta));                       \
        double _tu_float_expected = (double)(expected);                       \
        double _tu_float_actual = (double)(actual);                           \
        double _tu_float_diff = fabs(_tu_float_actual - _tu_float_expected);  \
        if (!(_tu_float_diff <= _tu_float_delta)) {                           \
            printf("[ASSERT] %s:%d: expected %.6f +/- %.6f, got %.6f\n",     \
                   __FILE__, __LINE__, _tu_float_expected,                    \
                   _tu_float_delta, _tu_float_actual);                        \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#define TEST_ASSERT_STR_EQUAL(expected, actual)                               \
    do {                                                                      \
        const char *_tu_str_expected = (expected);                            \
        const char *_tu_str_actual = (actual);                                \
        if (_tu_str_expected == NULL || _tu_str_actual == NULL) {             \
            if (_tu_str_expected != _tu_str_actual) {                         \
                printf("[ASSERT] %s:%d: expected %s, got %s\n",              \
                       __FILE__, __LINE__,                                    \
                       (_tu_str_expected != NULL) ? _tu_str_expected : "(null)", \
                       (_tu_str_actual != NULL) ? _tu_str_actual : "(null)"); \
                return 1;                                                     \
            }                                                                 \
        } else if (strcmp(_tu_str_expected, _tu_str_actual) != 0) {           \
            printf("[ASSERT] %s:%d: expected \"%s\", got \"%s\"\n",          \
                   __FILE__, __LINE__, _tu_str_expected, _tu_str_actual);     \
            return 1;                                                         \
        }                                                                     \
    } while (0)

#undef TEST_UTIL_UNUSED

#endif

#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

static void test_assert_int_equal_fail(void **state)
{
    (void)state; /* unused */
    assert_int_equal(0, 1);
    assert_int_equal(-1, -2);
    assert_int_equal(INTMAX_MIN, INTMAX_MAX);
    assert_int_equal(INTMAX_MAX, INTMAX_MIN);
}

static void test_assert_int_not_equal_fail(void **state)
{
    (void)state; /* unused */
    assert_int_not_equal(0, 0);
    assert_int_not_equal(1, 1);
    assert_int_not_equal(INTMAX_MIN, INTMAX_MIN);
    assert_int_not_equal(INTMAX_MAX, INTMAX_MAX);
}

static void test_assert_uint_equal_fail(void **state)
{
    (void)state; /* unused */
    assert_uint_equal(0, 1);
    assert_uint_equal(1, 2);
    assert_uint_equal(0, UINTMAX_MAX);
}

static void test_assert_uint_not_equal_fail(void **state)
{
    (void)state; /* unused */
    assert_uint_not_equal(0, 0);
    assert_uint_not_equal(1, 1);
    assert_uint_not_equal(UINTMAX_MAX, UINTMAX_MAX);
}

int main(void) {
    const struct CMUnitTest integer_tests[] = {
        cmocka_unit_test(test_assert_int_equal_fail),
        cmocka_unit_test(test_assert_int_not_equal_fail),
        cmocka_unit_test(test_assert_uint_equal_fail),
        cmocka_unit_test(test_assert_uint_not_equal_fail),
    };

    return cmocka_run_group_tests(integer_tests, NULL, NULL);
}


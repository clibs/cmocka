#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

static void test_assert_int_in_range_fail_1(void **state)
{
    (void)state; /* unused */
    assert_int_in_range(0, 1, 2);
}

static void test_assert_int_in_range_fail_2(void **state)
{
    (void)state; /* unused */
    assert_int_in_range(2, -1, 1);
}

static void test_assert_int_in_range_fail_3(void **state)
{
    (void)state; /* unused */
    assert_int_in_range(0, 1, INTMAX_MAX);
}

static void test_assert_int_in_range_fail_4(void **state)
{
    (void)state; /* unused */
    assert_int_in_range(0, INTMAX_MIN, -1);
}

int main(void) {
    const struct CMUnitTest range_fail_tests[] = {
        cmocka_unit_test(test_assert_int_in_range_fail_1),
        cmocka_unit_test(test_assert_int_in_range_fail_2),
        cmocka_unit_test(test_assert_int_in_range_fail_3),
        cmocka_unit_test(test_assert_int_in_range_fail_4),
    };

    return cmocka_run_group_tests(range_fail_tests, NULL, NULL);
}

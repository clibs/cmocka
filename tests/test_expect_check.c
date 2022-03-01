#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

static void mock_test_a(int value)
{
    check_expected(value);
}

int custom_checker(uintmax_t param, uintmax_t check);
int custom_checker(uintmax_t param, uintmax_t check)
{
    assert_int_equal(param, check);
    return 1;
}

static void test_expect_check(void **state)
{
    (void)state; /* unused */
    expect_check(mock_test_a, value, custom_checker, 0);
    mock_test_a(0);
}

static void test_expect_check_count(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, 0, 2);
    mock_test_a(0);
    mock_test_a(0);
}

static void test_expect_check_count_always(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, 0, EXPECT_ALWAYS);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
}

static void test_expect_check_count_maybe_1(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, 0, EXPECT_MAYBE);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
    mock_test_a(0);
}

static void test_expect_check_count_maybe_2(void **state)
{
    (void)state; /* unused */
    expect_check_count(mock_test_a, value, custom_checker, 0, EXPECT_MAYBE);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_expect_check),
        cmocka_unit_test(test_expect_check_count),
        cmocka_unit_test(test_expect_check_count_always),
        cmocka_unit_test(test_expect_check_count_maybe_1),
        cmocka_unit_test(test_expect_check_count_maybe_2)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

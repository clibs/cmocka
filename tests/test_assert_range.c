#include "config.h"

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>
#include <cmocka_private.h>

static void test_assert_int_in_range(void **state)
{
    (void)state; /* unused */
    assert_int_in_range(0, 0, 1);
    assert_int_in_range(1, 0, 1);
    assert_int_in_range(0, -1, 1);
    assert_int_in_range(0, INTMAX_MIN, INTMAX_MAX);
}

int main(void) {
    const struct CMUnitTest range_tests[] = {
        cmocka_unit_test(test_assert_int_in_range),
    };

    return cmocka_run_group_tests(range_tests, NULL, NULL);
}

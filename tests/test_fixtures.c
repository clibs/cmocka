#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>

static int setup_only(void **state)
{
    *state = malloc(1);

    return 0;
}

static int teardown_only(void **state)
{
    free(*state);

    return 0;
}

static void malloc_setup_test(void **state)
{
    assert_non_null(*state);
    free(*state);
}

static void malloc_teardown_test(void **state)
{
    *state = malloc(1);
    assert_non_null(*state);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test_setup(malloc_setup_test, setup_only),
        cmocka_unit_test_setup(malloc_setup_test, setup_only),
        cmocka_unit_test_teardown(malloc_teardown_test, teardown_only),
        cmocka_unit_test_teardown(malloc_teardown_test, teardown_only),
        cmocka_unit_test_teardown(malloc_teardown_test, teardown_only),
        cmocka_unit_test_teardown(malloc_teardown_test, teardown_only),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <stdlib.h>

static void setup_only(void **state)
{
    *state = malloc(1);
}

static void teardown_only(void **state)
{
    free(*state);
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
    const UnitTest tests[] = {
        unit_test_setup(malloc_setup_test, setup_only),
        unit_test_setup(malloc_setup_test, setup_only),
        unit_test_teardown(malloc_teardown_test, teardown_only),
        unit_test_teardown(malloc_teardown_test, teardown_only),
        unit_test_teardown(malloc_teardown_test, teardown_only),
        unit_test_teardown(malloc_teardown_test, teardown_only),
    };

    return run_tests(tests);
}

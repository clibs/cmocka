/* Use the unit test allocators */
#define UNIT_TESTING 1

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

static void group_setup(void **state)
{
    int *answer = malloc(sizeof(int));
    assert_non_null(answer);
    *answer = 42;

    *state = answer;
}

static void group_teardown(void **state)
{
    int *answer = (int *)*state;

    free(answer);
}

static void test_value_equal(void **state)
{
    int a = *((int *)*state);

    assert_int_equal(a, 42);
}

static void test_value_range(void **state)
{
    int a = *((int *)*state);

    assert_in_range(a, 0, 100);
}

int main(void) {
    const UnitTest tests[] = {
        group_test_setup(group_setup),
        unit_test(test_value_equal),
        unit_test(test_value_range),
        group_test_teardown(group_teardown),
    };

    return run_group_tests(tests);
}

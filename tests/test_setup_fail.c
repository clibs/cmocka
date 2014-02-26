#define UNIT_TESTING 1

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

static void setup_fail(void **state) {
    *state = NULL;

    /* We need to fail in setup */
    assert_non_null(NULL);
}

static void int_test_ignored(void **state) {
    /* should not be called */
    assert_non_null(*state);
}

static void setup_ok(void **state) {
    int *answer = malloc(sizeof(int));

    assert_non_null(answer);
    *answer = 42;

    *state = answer;
}

/* A test case that does check if an int is equal. */
static void int_test_success(void **state) {
    int *answer = *state;

    assert_int_equal(*answer, 42);
}

static void teardown(void **state) {
    free(*state);
}

int main(void) {
    const UnitTest tests[] = {
        unit_test_setup_teardown(int_test_ignored, setup_fail, teardown),
        unit_test_setup_teardown(int_test_success, setup_ok, teardown),
    };

    return run_tests(tests);
}

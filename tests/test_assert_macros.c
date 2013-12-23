#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/**************************************
 *** assert_return_code
 **************************************/
static void test_assert_return_code(void **state)
{
    struct stat sb;
    int rc;

    (void)state; /* unused */

    rc = stat(".", &sb);
    assert_return_code(rc, 0);

    assert_true(S_ISDIR(sb.st_mode));
}

int main(void) {
    const UnitTest tests[] = {
        unit_test(test_assert_return_code),
    };
    return run_tests(tests);
}

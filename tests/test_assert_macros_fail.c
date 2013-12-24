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
static void test_assert_return_code_fail(void **state)
{
    int fd;

    (void)state; /* unused */

    fd = open("this_file_doesnt_exist.cmocka", 0);
    assert_return_code(fd, errno);

    if (fd >= 0) {
        close(fd);
    }
}

int main(void) {
    const UnitTest tests[] = {
        unit_test(test_assert_return_code_fail),
    };
    return run_tests(tests);
}

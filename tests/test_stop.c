/*
 * Copyright 2022 Carsten Lange
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <stdint.h>
#include <cmocka.h>

static void mock_test_a_called(void)
{
    function_called();
}

static void mock_test_b_called(void)
{
    function_called();
}

static void test_check_stop(void **state) {
    (void)state; /* unused */

    expect_function_call(mock_test_a_called);

    mock_test_a_called();
    stop();
    mock_test_b_called(); /* not called due to calling stop() */
}


int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_check_stop),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

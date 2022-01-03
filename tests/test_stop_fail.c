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

static void mock_test_a_expected(int value)
{
    check_expected(value);
}

static void mock_test_b_expected(int value)
{
    check_expected(value);
}

static int mock_test_a_will_return(void)
{
   return mock_type(int);
}

static int mock_test_b_will_return(void)
{
    return mock_type(int);
}

static void test_stop_fails_when_not_returned_values_exist(void **state) {
    (void)state; /* unused */

    will_return(mock_test_a_will_return, 5);
    will_return(mock_test_b_will_return, 7);

    (void) mock_test_a_will_return();
    stop();
    (void) mock_test_b_will_return();
}

static void test_stop_fails_when_not_checked_values_exist(void **state) {
    (void)state; /* unused */

    expect_value(mock_test_a_expected, value, 5);
    expect_value(mock_test_b_expected, value, 7);

    mock_test_a_expected(5);
    stop();
    mock_test_b_expected(7);
}

static void test_stop_fails_when_unmade_expected_call_exist(void **state) {
    (void)state; /* unused */

   expect_function_call(mock_test_a_called);
   expect_function_call(mock_test_b_called);

    mock_test_a_called();
    stop();
    mock_test_b_called();
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_stop_fails_when_not_returned_values_exist),
        cmocka_unit_test(test_stop_fails_when_not_checked_values_exist),
        cmocka_unit_test(test_stop_fails_when_unmade_expected_call_exist),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}

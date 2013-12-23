project(tests C)

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
)

set(CMOCKA_TESTS
    test_fixtures
    test_assert_macros
    test_assert_macros_fail
    test_exception_handler)

foreach(_CMOCKA_TEST ${CMOCKA_TESTS})
    add_cmocka_test(${_CMOCKA_TEST} ${_CMOCKA_TEST}.c ${CMOCKA_SHARED_LIBRARY})
endforeach()

### Exceptions

# test_assert_macros_fail
set_tests_properties(
    test_assert_macros_fail
        PROPERTIES
        PASS_REGULAR_EXPRESSION
        "\\[  FAILED  \\] 1 test"
)

# test_exception_handler
set_tests_properties(
    test_exception_handler
        PROPERTIES
        PASS_REGULAR_EXPRESSION
        "Test failed with exception: (Segmentation fault|Segmentation Fault|11)"
)

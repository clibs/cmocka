project(cmocka-examples C CXX)

include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMOCKA_PUBLIC_INCLUDE_DIRS}
)

add_definitions(-DUNIT_TESTING=1)

add_executable(calculator_test calculator.c calculator_test.c)
target_link_libraries(calculator_test ${CMOCKA_SHARED_LIBRARY})

add_executable(allocate_module_test allocate_module.c allocate_module_test.c)
target_link_libraries(allocate_module_test ${CMOCKA_SHARED_LIBRARY})

add_executable(assert_macro_test assert_macro.c assert_macro_test.c)
target_link_libraries(assert_macro_test ${CMOCKA_SHARED_LIBRARY})

add_executable(customer_database_test customer_database.c customer_database_test.c)
target_link_libraries(customer_database_test ${CMOCKA_SHARED_LIBRARY})

add_executable(key_value_test key_value.c key_value_test.c)
target_link_libraries(key_value_test ${CMOCKA_SHARED_LIBRARY})

add_executable(product_database_test product_database.c product_database_test.c)
target_link_libraries(product_database_test ${CMOCKA_SHARED_LIBRARY})

add_executable(run_tests run_tests.c)
target_link_libraries(run_tests ${CMOCKA_SHARED_LIBRARY})

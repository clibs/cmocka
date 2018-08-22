include_directories(
    ${CMAKE_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMOCKA_PUBLIC_INCLUDE_DIRS}
)

add_library(proc_uptime proc_uptime.c)

add_executable(uptime uptime.c)
target_compile_options(uptime PRIVATE ${DEFAULT_C_COMPILE_FLAGS})
target_link_libraries(uptime proc_uptime)

add_executable(test_uptime test_uptime.c)
target_compile_options(test_uptime PRIVATE ${DEFAULT_C_COMPILE_FLAGS})
target_link_libraries(test_uptime ${CMOCKA_SHARED_LIBRARY})

set_target_properties(test_uptime
    PROPERTIES
        LINK_FLAGS "-Wl,--wrap=uptime")

add_test(test_uptime ${CMAKE_CURRENT_BINARY_DIR}/test_uptime)

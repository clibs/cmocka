# - add_cmocka_test(test_name test_source linklib1 ... linklibN)

# Copyright (c) 2007      Daniel Gollub <dgollub@suse.de>
# Copyright (c) 2007-2018 Andreas Schneider <asn@cryptomilk.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

enable_testing()
include(CTest)

if (CMAKE_CROSSCOMPILING)
    if (WIN32)
        find_program(WINE_EXECUTABLE
                     NAMES wine)
        set(TARGET_SYSTEM_EMULATOR ${WINE_EXECUTABLE})
    endif()
endif()

function(ADD_CMOCKA_TEST _testName _testSource)
    add_executable(${_testName} ${_testSource})

    target_link_libraries(${_testName} ${ARGN})

    add_test(${_testName} ${TARGET_SYSTEM_EMULATOR} ${CMAKE_CURRENT_BINARY_DIR}/${_testName}${CMAKE_EXECUTABLE_SUFFIX})
endfunction (ADD_CMOCKA_TEST)

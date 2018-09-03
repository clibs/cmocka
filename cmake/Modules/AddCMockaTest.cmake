# - add_cmocka_test(test_name test_source linklib1 ... linklibN)

# Copyright (c) 2007      Daniel Gollub <dgollub@suse.de>
# Copyright (c) 2007-2018 Andreas Schneider <asn@cryptomilk.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

enable_testing()
include(CTest)

if ((CMAKE_C_COMPILER_ID MATCHES "(GNU|Clang)") AND NOT MINGW)
    # Activate with: -DCMAKE_BUILD_TYPE=Profiling
    set(CMAKE_C_FLAGS_PROFILING "-g -O0 -fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the C compiler during PROFILING builds.")
    set(CMAKE_CXX_FLAGS_PROFILING "-g -O0 -fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the CXX compiler during PROFILING builds.")
    set(CMAKE_SHARED_LINKER_FLAGS_PROFILING "-fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the linker during the creation of shared libraries during PROFILING builds.")
    set(CMAKE_MODULE_LINKER_FLAGS_PROFILING "-fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the linker during the creation of shared libraries during PROFILING builds.")
    set(CMAKE_EXEC_LINKER_FLAGS_PROFILING "-fprofile-arcs -ftest-coverage"
        CACHE STRING "Flags used by the linker during PROFILING builds.")

    # Activate with: -DCMAKE_BUILD_TYPE=AddressSanitizer
    set(CMAKE_C_FLAGS_ADDRESSSANITIZER "-g -O1 -fsanitize=address -fno-omit-frame-pointer"
        CACHE STRING "Flags used by the C compiler during ADDRESSSANITIZER builds.")
    set(CMAKE_CXX_FLAGS_ADDRESSSANITIZER "-g -O1 -fsanitize=address -fno-omit-frame-pointer"
        CACHE STRING "Flags used by the CXX compiler during ADDRESSSANITIZER builds.")
    set(CMAKE_SHARED_LINKER_FLAGS_ADDRESSSANITIZER "-fsanitize=address"
        CACHE STRING "Flags used by the linker during the creation of shared libraries during ADDRESSSANITIZER builds.")
    set(CMAKE_MODULE_LINKER_FLAGS_ADDRESSSANITIZER "-fsanitize=address"
        CACHE STRING "Flags used by the linker during the creation of shared libraries during ADDRESSSANITIZER builds.")
    set(CMAKE_EXEC_LINKER_FLAGS_ADDRESSSANITIZER "-fsanitize=address"
        CACHE STRING "Flags used by the linker during ADDRESSSANITIZER builds.")
endif()

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

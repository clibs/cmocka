# Required cmake version
cmake_minimum_required(VERSION 3.5.0)
cmake_policy(SET CMP0048 NEW)

# Specify search path for CMake modules to be loaded by include() 
# and find_package()
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

### Add defaults for cmake
# These defaults need to be included before the project() call.
include(DefineCMakeDefaults)

# This will provide -DCMAKE_BUILD_TYPE=Profiling
# and -DCMAKE_BUILD_TYPE=AddressSanitizer
include(DefineCompilerFlags)
include(DefinePlatformDefaults)

project(cmocka VERSION 1.1.2 LANGUAGES C)

# global needed variables
set(APPLICATION_NAME ${PROJECT_NAME})

# SOVERSION scheme: CURRENT.AGE.REVISION
#   If there was an incompatible interface change:
#     Increment CURRENT. Set AGE and REVISION to 0
#   If there was a compatible interface change:
#     Increment AGE. Set REVISION to 0
#   If the source code was changed, but there were no interface changes:
#     Increment REVISION.
set(LIBRARY_VERSION "0.5.0")
set(LIBRARY_SOVERSION "0")

# include cmake files
include(GNUInstallDirs)
include(DefineOptions.cmake)
include(CPackConfig.cmake)
include(CompilerChecks.cmake)

# disallow in-source build
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build("${PROJECT_NAME} requires an out of source build. Please create a separate build directory and run 'cmake /path/to/${PROJECT_NAME} [options]' there.")

# config.h checks
include(ConfigureChecks.cmake)
configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

# MinGW DLL Naming Workaround
if (MINGW)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif (MINGW)

# check subdirectories
add_subdirectory(doc)
add_subdirectory(include)
add_subdirectory(src)

if (UNIT_TESTING)
    include(AddCMockaTest)
    add_subdirectory(tests)
endif (UNIT_TESTING)

add_subdirectory(example)

# pkg-config file
configure_file(cmocka.pc.cmake ${CMAKE_CURRENT_BINARY_DIR}/cmocka.pc)
install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/cmocka.pc
  DESTINATION
  ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  COMPONENT
    pkgconfig
)

# cmake config files
set(CMOCKA_LIBRARY_NAME ${CMAKE_SHARED_LIBRARY_PREFIX}${PROJECT_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})

configure_file(${PROJECT_NAME}-config.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake @ONLY)
configure_file(${PROJECT_NAME}-config-version.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake @ONLY)
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
    DESTINATION
        ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
    COMPONENT
        devel
)

# Add 'make dist' target which makes sure to invoke cmake before
add_custom_target(dist
                  COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

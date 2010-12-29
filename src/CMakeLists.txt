project(cmockery-library C)

set(CMOCKERY_PUBLIC_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/src/google
    CACHE INTERNAL "cmockery public include directories"
)

set(CMOCKERY_PRIVATE_INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}
)

set(CMOCKERY_SHARED_LIBRARY
    cmockery_shared
    CACHE INTERNAL "cmockery shared library"
)

if (WITH_STATIC_LIB)
    set(CMOCKERY_STATIC_LIBRARY
        cmockery_static
        CACHE INTERNAL "cmockery static library"
    )
endif (WITH_STATIC_LIB)

set(CMOCKERY_LINK_LIBRARIES
    ${CMOCKERY_REQUIRED_LIBRARIES}
    CACHE INTERNAL "cmockery link libraries"
)

set(cmockery_SRCS
    cmockery.c
)

if (WIN32)
    set(cmockery_SRCS
        ${cmockery_SRCS}
        cmockery.def
    )
endif (WIN32)

include_directories(
    ${CMOCKERY_PUBLIC_INCLUDE_DIRS}
    ${CMOCKERY_PRIVATE_INCLUDE_DIRS}
)

add_definitions(-DHAVE_CONFIG_H=1)

add_library(${CMOCKERY_SHARED_LIBRARY} SHARED ${cmockery_SRCS})

target_link_libraries(${CMOCKERY_SHARED_LIBRARY} ${CMOCKERY_LINK_LIBRARIES})

set_target_properties(
    ${CMOCKERY_SHARED_LIBRARY}
        PROPERTIES
            VERSION
                ${LIBRARY_VERSION}
            SOVERSION
                ${LIBRARY_SOVERSION}
            OUTPUT_NAME
                cmockery
            DEFINE_SYMBOL
                CMOCKERY_EXPORTS
)

install(
    TARGETS ${CMOCKERY_SHARED_LIBRARY}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    COMPONENT libraries
)

if (WITH_STATIC_LIB)
    add_library(${CMOCKERY_STATIC_LIBRARY} STATIC ${cmockery_SRCS})

    set_target_properties(
        ${CMOCKERY_STATIC_LIBRARY}
            PROPERTIES
                VERSION
                    ${LIBRARY_VERSION}
                SOVERSION
                    ${LIBRARY_SOVERSION}
                OUTPUT_NAME
                    cmockery
    )

    install(
        TARGETS ${CMOCKERY_STATIC_LIBRARY}
        DESTINATION ${LIB_INSTALL_DIR}
        COMPONENT libraries
    )
endif (WITH_STATIC_LIB)

add_subdirectory(google)
add_subdirectory(example)

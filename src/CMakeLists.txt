project(cmocka-library C)

set(CMOCKA_PUBLIC_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    CACHE INTERNAL "cmocka public include directories"
)

set(CMOCKA_PRIVATE_INCLUDE_DIRS
    ${CMAKE_BINARY_DIR}
)

set(CMOCKA_SHARED_LIBRARY
    cmocka_shared
    CACHE INTERNAL "cmocka shared library"
)

if (WITH_STATIC_LIB)
    set(CMOCKA_STATIC_LIBRARY
        cmocka_static
        CACHE INTERNAL "cmocka static library"
    )
endif (WITH_STATIC_LIB)

set(CMOCKA_LINK_LIBRARIES
    ${CMOCKA_REQUIRED_LIBRARIES}
    CACHE INTERNAL "cmocka link libraries"
)

set(cmocka_SRCS
    cmocka.c
)

if (WIN32)
    set(cmocka_SRCS
        ${cmocka_SRCS}
        cmocka.def
    )
endif (WIN32)

include_directories(
    ${CMOCKA_PUBLIC_INCLUDE_DIRS}
    ${CMOCKA_PRIVATE_INCLUDE_DIRS}
)

add_definitions(-DHAVE_CONFIG_H=1)

add_library(${CMOCKA_SHARED_LIBRARY} SHARED ${cmocka_SRCS})

target_link_libraries(${CMOCKA_SHARED_LIBRARY} ${CMOCKA_LINK_LIBRARIES})

set_target_properties(
    ${CMOCKA_SHARED_LIBRARY}
        PROPERTIES
            VERSION
                ${LIBRARY_VERSION}
            SOVERSION
                ${LIBRARY_SOVERSION}
            OUTPUT_NAME
                cmocka
            DEFINE_SYMBOL
                CMOCKA_EXPORTS
)

install(
    TARGETS ${CMOCKA_SHARED_LIBRARY}
    RUNTIME DESTINATION ${BIN_INSTALL_DIR}
    LIBRARY DESTINATION ${LIB_INSTALL_DIR}
    ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
    COMPONENT libraries
)

if (WITH_STATIC_LIB)
    add_library(${CMOCKA_STATIC_LIBRARY} STATIC ${cmocka_SRCS})

    set_target_properties(
        ${CMOCKA_STATIC_LIBRARY}
            PROPERTIES
                VERSION
                    ${LIBRARY_VERSION}
                SOVERSION
                    ${LIBRARY_SOVERSION}
                OUTPUT_NAME
                    cmocka
    )

    install(
        TARGETS ${CMOCKA_STATIC_LIBRARY}
        DESTINATION ${LIB_INSTALL_DIR}
        COMPONENT libraries
    )
endif (WITH_STATIC_LIB)


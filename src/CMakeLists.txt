project(cmocka-library C)

set(CMOCKA_PLATFORM_INCLUDE CACHE PATH "Path to include directory for cmocka_platform.h")
mark_as_advanced(CMOCKA_PLATFORM_INCLUDE)

set(cmocka_library_name)
if (BUILD_SHARED_LIBS)
    set(CMOCKA_SHARED_LIBRARY
        cmocka
        CACHE INTERNAL "cmocka shared library"
    )
    set(cmocka_library_name ${CMOCKA_SHARED_LIBRARY})
else (BUILD_SHARED_LIBS)
    set(CMOCKA_STATIC_LIBRARY
        cmocka-static
        CACHE INTERNAL "cmocka static library"
    )
    set(cmocka_library_name ${CMOCKA_STATIC_LIBRARY})
endif (BUILD_SHARED_LIBS)

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

add_library(${cmocka_library_name} ${cmocka_SRCS})

target_include_directories(${cmocka_library_name}
                           PRIVATE
                               ${CMOCKA_PLATFORM_INCLUDE}
                               ${cmocka_BINARY_DIR}
                           PUBLIC
                               ${cmocka-header_SOURCE_DIR})


target_compile_options(${cmocka_library_name}
                       PRIVATE
                           ${DEFAULT_C_COMPILE_FLAGS}
                           -DHAVE_CONFIG_H)
if (CMOCKA_PLATFORM_INCLUDE)
    target_compile_options(${cmocka_library_name}
                           PRIVATE
                               -DCMOCKA_PLATFORM_INCLUDE)
endif()

target_link_libraries(${cmocka_library_name} ${CMOCKA_LINK_LIBRARIES})
set_property(TARGET
                 ${cmocka_library_name}
             PROPERTY
                 DEFINE_SYMBOL
                     CMOCKA_EXPORTS
)

set_property(TARGET
                 ${cmocka_library_name}
             PROPERTY
                VERSION
                    ${LIBRARY_VERSION}
)
set_property(TARGET
                 ${cmocka_library_name}
             PROPERTY
                SOVERSION
                    ${LIBRARY_SOVERSION})

set_property(TARGET
                ${cmocka_library_name}
             PROPERTY
                LINK_FLAGS
                "${DEFAULT_LINK_FLAGS}")

add_library(cmocka::cmocka ALIAS ${cmocka_library_name})

install(TARGETS
            ${cmocka_library_name}
        ARCHIVE DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION
            ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION
            ${CMAKE_INSTALL_BINDIR}
        COMPONENT
            ${PROJECT_NAME})


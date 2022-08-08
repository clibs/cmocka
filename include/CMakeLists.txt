project(cmocka-header C)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmocka_version.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/cmocka_version.h
               @ONLY)

install(FILES
            cmocka.h
            cmocka_pbc.h
            ${CMAKE_CURRENT_BINARY_DIR}/cmocka_version.h
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT
            ${PROJECT_NAME})

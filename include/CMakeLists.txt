project(cmocka-header C)

install(FILES
            cmocka.h
            cmocka_pbc.h
        DESTINATION
            ${CMAKE_INSTALL_INCLUDEDIR}
        COMPONENT
            ${PROJECT_NAME})

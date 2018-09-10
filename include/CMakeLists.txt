project(cmocka-header C)

install(FILES
            cmocka.h
            cmocka_pbc.h
        DESTINATION
            ${INCLUDE_INSTALL_DIR}
        COMPONENT
            ${PROJECT_NAME})

if (WITH_CMOCKERY_SUPPORT)
    install(FILES
                cmockery/cmockery.h
                cmockery/pbc.h
            DESTINATION
                ${INCLUDE_INSTALL_DIR}/cmockery
            COMPONENT
                ${PROJECT_NAME})
endif()

#
# Build the documentation
#
find_package(Doxygen)

if (DOXYGEN_FOUND)
    doxygen_add_docs(docs
                     ${cmocka-library_SOURCE_DIR}
                     ${CMAKE_CURRENT_SOURCE_DIR})
endif(DOXYGEN_FOUND)

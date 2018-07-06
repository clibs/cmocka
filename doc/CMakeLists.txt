#
# Build the documentation
#
find_package(Doxygen REQUIRED dot)

if (DOXYGEN_FOUND)
    doxygen_add_docs(docs
                     ${CMAKE_CURRENT_SOURCE_DIR})
endif(DOXYGEN_FOUND)

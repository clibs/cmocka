option(WITH_STATIC_LIB "Build with a static library" OFF)
option(WITH_CMOCKERY_SUPPORT "Install a cmockery header" OFF)
option(UNIT_TESTING "Build with unit testing" OFF)
option(PICKY_DEVELOPER "Build with picky developer flags" OFF)

if (WITH_STATIC_LIB)
    set(BUILD_STATIC_LIB ON)
endif (WITH_STATIC_LIB)

if (UNIT_TESTING)
  set(BUILD_STATIC_LIB ON)
endif (UNIT_TESTING)

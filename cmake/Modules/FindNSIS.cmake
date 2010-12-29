# - Try to find NSIS
# Once done this will define
#
#  NSIS_FOUND - system has NSIS
#  NSIS_MAKE - NSIS creator executable
#
#  Copyright (c) 2010 Andreas Schneider <mail@cynapses.org>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#

if (NSIS_MAKE)
  # in cache already
  set(NSIS_FOUND TRUE)
elseif (NSIS_MAKE)
    find_program(NSIS_MAKE
        NAMES
            makensis
        PATHS
            ${_NSIS_DIR}
            ${_NSIS_DIR}/Bin
            $ENV{PROGRAMFILES}/NSIS
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(NSIS DEFAULT_MSG NSIS_MAKE)

    mark_as_advanced(NSIS_MAKE)
endif (NSIS_MAKE)

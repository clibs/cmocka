project(cmockery-headers C)

set(cmockery_HDRS
  cmockery.h
)

install(
  FILES
    ${cmockery_HDRS}
  DESTINATION
    ${INCLUDE_INSTALL_DIR}/google
  COMPONENT
    headers
)

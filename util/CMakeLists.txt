set(UTIL_SOURCES
  xerror.c
  common.c
  file.c
  process.c
  elf.c
  embeddable.c
)
list(TRANSFORM UTIL_SOURCES PREPEND src/)

add_library(util
  OBJECT
  ${UTIL_SOURCES}
  ${UTIL_ARCH_SOURCES}
)

target_sources(util
  PRIVATE $<$<CONFIG:DEBUG>:src/debug.c>
)

target_include_directories(util
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE internal/include
  PRIVATE arch/${_ARCH}
)

set_target_properties(util
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
)

install(
  TARGETS util
  EXPORT Infect
)

install(
  FILES
    include/util/error_codes.inc
    include/util/error_codes.h
  DESTINATION include/infect
)

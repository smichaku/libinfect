set(INFECT_SOURCES
  remote.c
  remote-mem.c
  remote-dlfcn.c
  remote-stdlib.c
  remote-module.c
  resolve.c
  inject-code.c
  inject-dso.c
  inject-dso-unsafe.c
  inject-module.c
)
list(TRANSFORM INFECT_SOURCES PREPEND src/)

include_directories(
  internal/include
)

set(INFECT_ARCH_SOURCES
  remote.c
)
list(TRANSFORM INFECT_ARCH_SOURCES PREPEND arch/${_ARCH}/)

add_library(infect
  STATIC
  ${INFECT_SOURCES}
  ${INFECT_ARCH_SOURCES}
)

add_library(Infect::infect ALIAS infect)

target_include_directories(infect
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE arch/${_ARCH}
)

target_link_libraries(infect
  PRIVATE util
)

install_library(infect)

add_subdirectory(modules)

if (LIBINFECT_INCLUDE_TESTS)
  add_subdirectory(tests)
endif ()

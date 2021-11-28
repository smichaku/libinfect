include(InfectBaremetal)

set(BAREMETAL_SOURCES
  atomic.c
  futex.c
  signal.c
  file.c
  exit.c
  mman.c
  string.c
  process.c
  thread.c
  alloc.c
  format.c
  printf.c
  putchar.c
)
list(TRANSFORM BAREMETAL_SOURCES PREPEND src/)

set(BAREMETAL_ARCH_SOURCES
  sys_clone.S
)
list(TRANSFORM BAREMETAL_ARCH_SOURCES PREPEND arch/${_ARCH}/)

add_library(baremetal
  STATIC
  ${BAREMETAL_SOURCES}
  ${BAREMETAL_ARCH_SOURCES}
)

add_library(Infect::baremetal ALIAS baremetal)

target_sources(baremetal
  PRIVATE $<$<CONFIG:DEBUG>:src/debug.c>
)

target_include_directories(baremetal
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE internal/include
  PRIVATE arch/${_ARCH}
)

target_compile_options(baremetal
  PRIVATE ${BAREMETAL_COMPILE_OPTIONS}
)

target_link_options(baremetal
  PRIVATE ${BAREMETAL_LINK_OPTIONS}
)

set_target_properties(baremetal
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
)

install_library(baremetal)

if (LIBINFECT_INCLUDE_TESTS)
  add_subdirectory(tests)
endif ()

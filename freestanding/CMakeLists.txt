include(InfectFreestanding)

set(FREESTANDING_SOURCES
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
list(TRANSFORM FREESTANDING_SOURCES PREPEND src/)

set(FREESTANDING_ARCH_SOURCES
  sys_clone.S
)
list(TRANSFORM FREESTANDING_ARCH_SOURCES PREPEND arch/${_ARCH}/)

add_library(freestanding
  OBJECT
  ${FREESTANDING_SOURCES}
  ${FREESTANDING_ARCH_SOURCES}
)

add_library(Infect::freestanding ALIAS freestanding)

target_sources(freestanding
  PRIVATE $<$<CONFIG:DEBUG>:src/debug.c>
)

target_include_directories(freestanding
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE internal/include
  PRIVATE arch/${_ARCH}
)

target_compile_options(freestanding
  PRIVATE ${FREESTANDING_COMPILE_OPTIONS}
)

target_link_options(freestanding
  PRIVATE ${FREESTANDING_LINK_OPTIONS}
)

set_target_properties(freestanding
  PROPERTIES
    POSITION_INDEPENDENT_CODE TRUE
)

install_library(freestanding)

if (LIBINFECT_INCLUDE_TESTS)
  add_subdirectory(tests)
endif ()

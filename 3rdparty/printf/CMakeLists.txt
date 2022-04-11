# printf is currently only used by libfreestanding and we need to build it with
# the appropriate compile options. We therefore don't specify it as a library
# here but instead just set some global cache variables with the paths of the
# sources and include directory.

set(PRINTF_SOURCES printf.c)
list(TRANSFORM PRINTF_SOURCES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
set(PRINTF_SOURCES
  ${PRINTF_SOURCES}
  CACHE
  PATH
  ""
  FORCE
)

set(PRINTF_INCLUDE_DIR
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  CACHE
  PATH
  ""
  FORCE
)

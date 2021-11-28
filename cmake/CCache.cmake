include_guard()

include(CMakeDependentOption)

find_program(CCACHE_PROGRAM ccache)

cmake_dependent_option(LIBINFECT_ENABLE_CCACHE
  "Enable ccache build" OFF "CCACHE_PROGRAM" OFF
)

if (LIBINFECT_ENABLE_CCACHE)
  message(STATUS "Using ccache as compile launcher")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${CCACHE_PROGRAM})
endif ()

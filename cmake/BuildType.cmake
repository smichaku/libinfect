include_guard()

set(DEFAULT_BUILD_TYPE Debug)

if (NOT CMAKE_BUILD_TYPE)
  message(STATUS "Build type not specified, defaulting to ${DEFAULT_BUILD_TYPE}")
  set(CMAKE_BUILD_TYPE ${DEFAULT_BUILD_TYPE} CACHE STRING "Build type" FORCE)
endif ()

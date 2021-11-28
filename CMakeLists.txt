cmake_minimum_required(VERSION 3.13)

project(infect
  LANGUAGES C CXX ASM
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_COLOR_MAKEFILE ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm.*")
  set(_ARCH arm)
elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES "x64|x86_64|amd64")
  set(_ARCH x86)
else ()
  message("Processor ${CMAKE_SYSTEM_PROCESSOR} is not supported")
endif ()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(InfectPrograms)
include(BuildType)
include(CompilerWarnings)
include(CCache)
include(Backtrace)
include(Sanitizers)
include(Packages)
include(Embed)
include(Install)

option(LIBINFECT_INCLUDE_TESTS "Generate tests for libinfect." ON)
if (LIBINFECT_INCLUDE_TESTS)
  enable_testing()
endif ()

add_subdirectory(3rdparty)
add_subdirectory(util)
add_subdirectory(util++)
add_subdirectory(baremetal)
add_subdirectory(infect)

option(LIBINFECT_INCLUDE_TOOLS "Generate tools." ON)
if (LIBINFECT_INCLUDE_TOOLS)
  add_subdirectory(tools)
endif ()

option(LIBINFECT_INCLUDE_PLUGINS "Generate plugins." ON)
if (LIBINFECT_INCLUDE_PLUGINS)
  add_subdirectory(plugins)
endif ()

install_infect_export()

#
# A DSO injectable which will be used in tests
#
add_library(infect.tests.dso
  MODULE
  injectable.dso.c
)

set_target_properties(infect.tests.dso
  PROPERTIES
    PREFIX ""
)

target_link_libraries(infect.tests.dso
  PRIVATE util
)

#
# A module injectable which will be used in tests
#
add_infect_module(infect.tests.module
  injectable.module.c
)

target_link_libraries(infect.tests.module
  PRIVATE baremetal
)

#
# The test suite
#
set(INFECT_TESTS_SOURCES
  victim.c
  main.cpp
  remote.cpp
  inject-code.cpp
  inject.cpp
)

add_executable(infect.tests
  ${INFECT_TESTS_SOURCES}
)

target_link_libraries(infect.tests
  PRIVATE std::filesystem
  PRIVATE Catch2
  PRIVATE util
  PRIVATE util++
  PRIVATE infect
)

target_link_options(infect.tests
  PRIVATE "-Wl,-wrap=process_vm_readv"
)

enable_backtrace(infect.tests)

embed_in_target(infect.tests infect.tests.dso)
embed_in_target(infect.tests infect.tests.module)

add_test(
  NAME infect.tests
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  COMMAND infect.tests
)

set(BAREMETAL_TESTS_SOURCES
  main.cpp
  thread.cpp
)

add_executable(baremetal.tests
  ${BAREMETAL_TESTS_SOURCES}
)

set_target_properties(baremetal.tests
  PROPERTIES LINKER_LANGUAGE CXX
)

target_link_libraries(baremetal.tests
  PRIVATE Catch2
  PRIVATE baremetal
)

enable_backtrace(baremetal.tests)

add_test(
  NAME baremetal.tests
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  COMMAND baremetal.tests
)

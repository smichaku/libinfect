set(FREESTANDING_TESTS_SOURCES
  main.cpp
  thread.cpp
)

add_executable(freestanding.tests
  ${FREESTANDING_TESTS_SOURCES}
)

set_target_properties(freestanding.tests
  PROPERTIES LINKER_LANGUAGE CXX
)

target_link_libraries(freestanding.tests
  PRIVATE Catch2
  PRIVATE freestanding
)

enable_backtrace(freestanding.tests)

add_test(
  NAME freestanding.tests
  WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
  COMMAND freestanding.tests
)

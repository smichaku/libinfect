set(INJECTOR_SOURCES
  main.cpp
  injector.cpp
)

add_executable(injector
  ${INJECTOR_SOURCES}
)

target_link_libraries(injector
  PRIVATE CLI11
  PRIVATE util
  PRIVATE infect
)

install_executable(injector)

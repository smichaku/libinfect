include(InfectModule)

set(MODULE_OUTPUT_DIRECTORY
  "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/modules"
  CACHE
  PATH
  ""
)

add_library(module_framework
  OBJECT
  init.c
  module-promise.c
)

add_library(Infect::module_framework ALIAS module_framework)

freestanding_target(module_framework)

set_source_files_properties(init.c
  PROPERTIES
    COMPILE_OPTIONS "-Wno-prio-ctor-dtor"
)

install_library(module_framework)

function (add_builtin_module TARGET SOURCES)
  add_infect_module(${TARGET} ${SOURCES})

  target_include_directories(${TARGET}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  )

  set_property(
    SOURCE modules.S
      TARGET_DIRECTORY infect
    APPEND
    PROPERTY
      OBJECT_DEPENDS "${MODULE_OUTPUT_DIRECTORY}/${TARGET}.so"
  )

  add_dependencies(infect ${TARGET})
endfunction ()

add_builtin_module(async_dlfcn
  async_dlfcn/async_dlfcn.c
)

target_sources(infect
  PRIVATE modules.S
)

target_include_directories(infect
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE ${MODULE_OUTPUT_DIRECTORY}
)

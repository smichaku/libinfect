function (add_infect_plugin TARGET SOURCES)
  add_infect_module(${TARGET} ${SOURCES})

  set_target_properties(${TARGET}
    PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIRECTORY}/plugins"
  )
endfunction ()

add_subdirectory(redirect)

include_guard()

function (add_infect_module TARGET SOURCES)
  add_library(${TARGET}
    MODULE
    ${SOURCES}
  )

  freestanding_target(${TARGET})

  target_link_libraries(${TARGET}
    PRIVATE Infect::module_framework
  )

  set_target_properties(${TARGET}
    PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY "${MODULE_OUTPUT_DIRECTORY}"
      PREFIX ""
  )

  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMAND ${OBJCOPY} --add-section .module_info=/dev/null $<TARGET_FILE:${TARGET}>
  )
endfunction ()

include_guard()

include(Scripts)

set(EMBED_SCRIPT
  ${BUILD_SCRIPTS_DIR}/embed
  CACHE PATH ""
)

function (embed_in_target TARGET EMBEDDABLE)
  if (NOT TARGET ${TARGET})
    message(FATAL_ERROR "Embedding target ${TARGET} does not exist")
  endif ()
  if (NOT TARGET ${EMBEDDABLE})
    message(FATAL_ERROR "Embeddable target ${EMBEDDABLE} does not exist")
  endif ()

  add_dependencies(${TARGET} ${EMBEDDABLE})

  # Trigger a rebuild of TARGET in case an embeddable is updated. We generate
  # an empty source file which depends on all embeddable targets and files and
  # set it as a new source of TARGET.
  set(EMBREADY_FILE ${CMAKE_CURRENT_BINARY_DIR}/${TARGET}.embready.c)
  add_custom_command(OUTPUT ${EMBREADY_FILE}
    DEPENDS $<TARGET_FILE:${EMBEDDABLE}>
    COMMAND ${CMAKE_COMMAND} -E touch ${EMBREADY_FILE}
  )
  set_source_files_properties(${EMBREADY_FILE}
    PROPERTIES
      COMPILE_FLAGS "-Wno-pedantic"
  )
  target_sources(${TARGET}
    PRIVATE ${EMBREADY_FILE}
  )

  # The final executable is created by applying the embed script with the
  # source and target executables and the files to be embedded.
  add_custom_command(TARGET ${TARGET}
    POST_BUILD
    COMMENT "Embedding ${EMBEDDABLE} in ${TARGET}"
    COMMAND
      ${EMBED_SCRIPT}
        ${OBJCOPY} $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${TARGET}> $<TARGET_FILE:${EMBEDDABLE}>
  )
endfunction ()

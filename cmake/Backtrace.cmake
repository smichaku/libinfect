include_guard()

function (enable_backtrace TARGET)
  get_target_property(TARGET_TYPE ${TARGET} TYPE)

  if (NOT TARGET_TYPE STREQUAL EXECUTABLE)
    msg(FATAL_ERROR "enable_backtrace() should only be used for executables")
  endif ()

  target_link_options(${TARGET}
    PRIVATE "-rdynamic"
  )
endfunction ()

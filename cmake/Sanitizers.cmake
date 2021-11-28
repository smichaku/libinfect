include_guard()

option(LIBINFECT_WITH_SANITIZERS "Build with sanitizers." OFF)
if (LIBINFECT_WITH_SANITIZERS)
  add_compile_options(
    $<$<CONFIG:DEBUG>:-fsanitize=address>
  )
  add_link_options(
    $<$<CONFIG:DEBUG>:-fsanitize=address>
  )
endif ()


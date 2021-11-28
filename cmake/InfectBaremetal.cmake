include_guard()

set(BAREMETAL_COMPILE_OPTIONS
  "-fno-plt;-ffreestanding;-fno-stack-protector;-fno-sanitize=all"
  CACHE
  STRING
  ""
  FORCE
)

set(BAREMETAL_LINK_OPTIONS
  "-Wl,-no-undefined;-nostartfiles;-nolibc;-fno-sanitize=all"
  CACHE
  STRING
  ""
  FORCE
)

function (baremetal_target TARGET)
  target_compile_options(${TARGET}
    PRIVATE ${BAREMETAL_COMPILE_OPTIONS}
  )

  target_link_options(${TARGET}
    PRIVATE ${BAREMETAL_LINK_OPTIONS}
  )

  target_link_libraries(${TARGET}
    PRIVATE Infect::baremetal
  )
endfunction ()

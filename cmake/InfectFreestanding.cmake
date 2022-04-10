include_guard()

set(FREESTANDING_COMPILE_OPTIONS
  "-fno-plt;-ffreestanding;-fno-stack-protector;-fno-sanitize=all"
  CACHE
  STRING
  ""
  FORCE
)

set(FREESTANDING_LINK_OPTIONS
  "-Wl,-no-undefined;-nostartfiles;-nolibc;-fno-sanitize=all"
  CACHE
  STRING
  ""
  FORCE
)

function (freestanding_target TARGET)
  target_compile_options(${TARGET}
    PRIVATE ${FREESTANDING_COMPILE_OPTIONS}
  )

  target_link_options(${TARGET}
    PRIVATE ${FREESTANDING_LINK_OPTIONS}
  )

  target_link_libraries(${TARGET}
    PRIVATE Infect::freestanding
  )
endfunction ()

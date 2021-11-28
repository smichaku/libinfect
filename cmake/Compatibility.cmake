include_guard()

include(CheckCCompilerFlag)

# add_supported_compile_option(OPTIONS)
#
# Conditionally add the given list of options if supported by the compiler.
function (add_supported_compile_options OPTIONS)
  foreach (opt IN LISTS OPTIONS)
    check_c_compiler_flag(${opt} COMPILER_HAS_${opt})
    if (COMPILER_HAS_${opt})
      add_compile_options("${opt}")
    endif ()
  endforeach ()
endfunction ()

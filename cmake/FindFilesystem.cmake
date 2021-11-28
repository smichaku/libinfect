include_guard()

include(FindPackageHandleStandardArgs)
include(CMakePushCheckState)
include(CheckIncludeFileCXX)
include(CheckCXXSourceCompiles)
include(StandardCXXLibrary)

standard_cxx_library(STANDARD_CXX_LIB)

cmake_push_check_state(RESET)
if (STANDARD_CXX_LIB)
  check_include_file_cxx(filesystem HAS_FS_HEADER)
  if (NOT HAS_FS_HEADER)
    check_include_file_cxx(experimental/filesystem HAS_EXP_FS_HEADER)
  endif ()

  if (HAS_FS_HEADER)
    set(HEADER filesystem)
    set(NAMESPACE std::filesystem)
  elseif (HAS_EXP_FS_HEADER)
    set(HEADER experimental/filesystem)
    set(NAMESPACE std::experimental::filesystem)
  endif ()

  if (HEADER)
    string(CONFIGURE [[
      #include <@HEADER@>

      int main()
      {
        return @NAMESPACE@::current_path().string().length();
      }
      ]] CODE @ONLY
    )

    check_cxx_source_compiles("${CODE}" FS_LINK_NOTHING)
    if (NOT FS_LINK_NOTHING)
      set(CMAKE_REQUIRED_LIBRARIES "${STANDARD_CXX_LIB}fs")
      check_cxx_source_compiles("${CODE}" FS_LINK_STDFS)
    endif ()

    if (FS_LINK_NOTHING OR FS_LINK_STDFS)
      set(FS_AVAILABLE TRUE)
      add_library(std::filesystem INTERFACE IMPORTED GLOBAL)
      if (FS_LINK_STDFS)
        target_link_libraries(std::filesystem
          INTERFACE "${STANDARD_CXX_LIB}fs"
        )
      endif ()
    endif ()
  endif ()
endif ()
cmake_pop_check_state()

find_package_handle_standard_args(Filesystem
  REQUIRED_VARS FS_AVAILABLE
)

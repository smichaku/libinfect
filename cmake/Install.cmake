set(INFECT_CONFIG_DIR
  ${CMAKE_CURRENT_LIST_DIR}/config
  CACHE PATH ""
)

set(
  INFECT_EXPORT_INTERMEDIATE_DIR
  ${CMAKE_BINARY_DIR}/CMakeFiles/Export/cmake
  CACHE PATH "" FORCE
)

set(INFECT_ARCHIVE_INSTALL_DIR
  lib/infect
  CACHE PATH ""
)
set(INFECT_LIBRARY_INSTALL_DIR
  lib/infect
  CACHE PATH ""
)
set(INFECT_OBJECT_INSTALL_DIR
  lib/infect
  CACHE PATH ""
)
set(INFECT_RUNTIME_INSTALL_DIR
  bin
  CACHE PATH ""
)
set(INFECT_CMAKE_INSTALL_DIR
  lib/cmake/infect
  CACHE PATH ""
)

# Get the build and install tree include directories for a given TARGET.
function (get_include_directories TARGET OUT_BUILD_INCLUDE_DIRS OUT_INSTALL_INCLUDE_DIR)
  get_target_property(INCLUDE_DIRS ${TARGET} INTERFACE_INCLUDE_DIRECTORIES)

  set(BUILD_INCLUDE_DIRS ${INCLUDE_DIRS})
  list(FILTER BUILD_INCLUDE_DIRS INCLUDE REGEX "BUILD_INTERFACE")
  list(TRANSFORM BUILD_INCLUDE_DIRS APPEND "/")

  string(REGEX MATCH "\\$<INSTALL_INTERFACE:(.*)>" MATCHED "${INCLUDE_DIRS}")
  set(INSTALL_INCLUDE_DIR ${CMAKE_MATCH_1})

  set(${OUT_BUILD_INCLUDE_DIRS} ${BUILD_INCLUDE_DIRS} PARENT_SCOPE)
  set(${OUT_INSTALL_INCLUDE_DIR} ${INSTALL_INCLUDE_DIR} PARENT_SCOPE)
endfunction ()

# install_headers(<target>)
#
# Install the headers of the given target. The target is expected to be set
# with the BUILD_INTERFACE and INSTALL_INTERFACE generator expressions to
# distinguish between the include path(s) in the build and install trees
# respectively. If the INSTALL_HEADERS_PATTERN property is set for the target
# only files matching this pattern are installed.
function (install_headers TARGET)
  get_target_property(TARGET_TYPE ${TARGET} TYPE)
  get_include_directories(${TARGET} BUILD_INCLUDE_DIRS INSTALL_INCLUDE_DIR)
  get_target_property(PUBLIC_HEADER ${TARGET} PUBLIC_HEADER)

  if (BUILD_INCLUDE_DIRS AND INSTALL_INCLUDE_DIR)
    if (NOT TARGET_TYPE STREQUAL INTERFACE_LIBRARY)
      get_target_property(INSTALL_HEADERS_PATTERN ${TARGET} INSTALL_HEADERS_PATTERN)
    endif ()
    if (NOT INSTALL_HEADERS_PATTERN)
      set(INSTALL_HEADERS_PATTERN "*")
    endif ()

    install(DIRECTORY ${BUILD_INCLUDE_DIRS}
      DESTINATION ${INSTALL_INCLUDE_DIR}
      FILES_MATCHING
        PATTERN ${INSTALL_HEADERS_PATTERN}
    )
  endif ()
endfunction ()

# install_library([EXPORT] <target>)
#
# Install the given target. The EXPORT option determines whether the library
# should also be exported for use by an external client project. If not
# exported the library is only available to satisfy linkage for other libraries
# which depend on it.
function (install_library TARGET)
  install(TARGETS ${TARGET}
    EXPORT Infect
    ARCHIVE DESTINATION ${INFECT_ARCHIVE_INSTALL_DIR}
    LIBRARY DESTINATION ${INFECT_LIBRARY_INSTALL_DIR}
    OBJECTS DESTINATION ${INFECT_OBJECT_INSTALL_DIR}
  )

  install_headers(${TARGET})
endfunction ()

# install_executable(<target>)
#
# Install the given executable target in the 'bin' directory.
function (install_executable TARGET)
  install(TARGETS ${TARGET}
    RUNTIME DESTINATION ${INFECT_RUNTIME_INSTALL_DIR}
  )
endfunction ()

#
# install_plugin(<target>)
#
# Install the given plugin in the 'plugin' directory.
function (install_plugin TARGET)
  install(TARGETS ${TARGET}
    LIBRARY DESTINATION ${INFECT_LIBRARY_INSTALL_DIR}/plugins
  )
endfunction ()

# install_infect_export()
#
# Install the Infect export as well as other CMake files and scripts
# required by the external projects.
function (install_infect_export)
  install(
    EXPORT Infect
    FILE InfectExport.cmake
    NAMESPACE Infect::
    DESTINATION ${INFECT_CMAKE_INSTALL_DIR}
  )

  set(EXPORTED_CMAKE_FILES InfectConfig InfectPrograms InfectFreestanding InfectModule)
  foreach (FILE ${EXPORTED_CMAKE_FILES})
    install(
      FILES "cmake/${FILE}.cmake"
      DESTINATION ${INFECT_CMAKE_INSTALL_DIR}
    )
  endforeach ()

  set(TOP_FILE "${INFECT_EXPORT_INTERMEDIATE_DIR}/Infect.cmake")
  configure_file(
    "${INFECT_CONFIG_DIR}/Infect.cmake.in"
    ${TOP_FILE}
    @ONLY
  )
  install(
    FILES ${TOP_FILE}
    DESTINATION ${INFECT_CMAKE_INSTALL_DIR}
  )
endfunction ()

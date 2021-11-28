include_guard()

include(Message)

function (standard_cxx_library OUTLIB)
  msg(CHECK_START "Detecting C++ standard library")

  set(SOURCE [[
      #include <vector>

      int main()
      {
        std::vector<int> v;

        return v.size();
      }
    ]]
  )

  set(SOURCE_FILE "${CMAKE_BINARY_DIR}/CMakeTmp/standard_cxx_library.cpp")
  set(EXECUTABLE_FILE "${CMAKE_BINARY_DIR}/CMakeTmp/standard_cxx_library")

  file(WRITE "${SOURCE_FILE}" "${SOURCE}\n")

  try_compile(
    RESULT
    ${CMAKE_BINARY_DIR}
    ${SOURCE_FILE}
    COPY_FILE ${EXECUTABLE_FILE}
  )

  execute_process(
    COMMAND ${READELF} -d ${EXECUTABLE_FILE}
    OUTPUT_VARIABLE DYNAMIC_SECTION
  )

  string(FIND "${DYNAMIC_SECTION}" "libstdc++.so" LIBSTDCXX)
  string(FIND "${DYNAMIC_SECTION}" "libc++.so" LIBCXX)

  if (NOT LIBSTDCXX STREQUAL "-1")
    msg(CHECK_PASS "found libstdc++")
    set(${OUTLIB} stdc++ PARENT_SCOPE)
  elseif (NOT LIBCXX STREQUAL "-1")
    msg(CHECK_PASS "found libc++")
    set(${OUTLIB} c++ PARENT_SCOPE)
  else ()
    message(CHECK_FAIL "not found")
    set(${OUTLIB} NOTFOUND PARENT_SCOPE)
  endif ()
endfunction ()

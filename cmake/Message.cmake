include_guard()

function (msg MODE TEXT)
  set(STACK ${_MSG_CHECK_STACK})
  list(LENGTH STACK STACK_LENGTH)

  math(EXPR LAST_IDX "${STACK_LENGTH} - 1")

  if (MODE STREQUAL CHECK_START)
    list(APPEND STACK ${TEXT})
    set(REALMODE STATUS)
  elseif (MODE STREQUAL CHECK_PASS OR MODE STREQUAL CHECK_FAIL)
    if (STACK_LENGTH EQUAL 0)
      message(
        FATAL_ERROR
          "msg() invoked with CHECK_PASS or CHECK_FAIL without a prior CHECK_START"
      )
    endif ()

    list(GET STACK ${LAST_IDX} OLD_TEXT)
    list(REMOVE_AT STACK ${LAST_IDX})
    set(REALMODE STATUS)
    set(TEXT "${OLD_TEXT} - ${TEXT}")
  endif ()

  set(_MSG_CHECK_STACK ${STACK} PARENT_SCOPE)

  message(${REALMODE} ${TEXT})
endfunction ()

include(InfectModule)

set(REDIRECT_SOURCES
  redirect.c
)

add_infect_plugin(redirect
  ${REDIRECT_SOURCES}
)

install_plugin(redirect)

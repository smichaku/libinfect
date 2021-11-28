include_guard()

include(Compatibility)

add_compile_options(
  -Werror -Wall -Wextra -Wpedantic
  -Wshadow
  -Wdouble-promotion
  -Wundef
)

add_supported_compile_options("-Wformat=2;-Wformat-truncation=2;-Wformat-overflow=2")

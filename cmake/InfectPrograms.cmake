include_guard()

find_program(OBJCOPY NAMES ${_ARCH}-linux-objcopy objcopy REQUIRED)
find_program(READELF NAMES ${_ARCH}-linux-readelf readelf REQUIRED)

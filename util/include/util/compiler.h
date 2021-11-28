#pragma once

#include <stdint.h>

#define unused       __attribute__((unused))
#define maybe_unused unused

#if UINTPTR_MAX > 0xffffffff
#define __64BIT
#else
#define __32BIT
#endif

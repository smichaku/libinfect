#pragma once

#include <stddef.h>

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)
#define GB(x) ((size_t) (x) << 30)

#define ALIGN(x, a)    ((x) & ~((a) - (1)))
#define ALIGN_UP(x, a) (((x) + (a) - (1)) & -(a))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

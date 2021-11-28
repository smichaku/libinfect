#pragma once

#include <stdint.h>

#ifdef NDEBUG
#define bm_debug_printf(...)
#else
/* clang-format off */
__attribute__ ((format (printf, 1, 2)))
void bm_debug_printf(const char *fmt, ...);
/* clang-format on */

static inline void bm_debug_crash(void)
{
    volatile int *a = (volatile int *) 0x140;

    *a = 0;
}

static inline void bm_debug_crash_at(uintptr_t a)
{
    *((volatile int *) a) = 0;
}
#endif

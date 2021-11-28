#include <util/debug.h>

#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

enum
{
    UNKNOWN,
    ON,
    OFF,
};

/* clang-format off */
__attribute__ ((format(printf, 1, 2)))
void debug_printf(const char *fmt, ...)
/* clang-format on */
{
    static int print_state = UNKNOWN;

    va_list vl;
    int state;

    state = __atomic_load_n(&print_state, __ATOMIC_RELAXED);
    if (state == UNKNOWN) {
        state = getenv("LIBINFECT_DEBUG_ENABLE") != NULL ? ON : OFF;
        __atomic_store_n(&print_state, state, __ATOMIC_RELAXED);
    }

    if (state == ON && fmt != NULL) {
        va_start(vl, fmt);
        vfprintf(stderr, fmt, vl);
        fflush(stderr);
        va_end(vl);
    }
}

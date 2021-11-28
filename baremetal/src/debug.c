#include <baremetal/debug.h>

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

#include <baremetal/compiler.h>
#include <baremetal/file.h>
#include <printf.h>

/* The printf.h header define a printf macro which will mess up the
 * __attribute__(format) ahead. */
#undef printf

/* clang-format off */
__attribute__((format(printf, 1, 2)))
void bm_debug_printf(const char *fmt, ...)
/* clang-format on */
{
    va_list vl;
    char buffer[1024];
    int printed;
    size_t written;

    va_start(vl, fmt);
    printed = vsnprintf(buffer, sizeof(buffer) - 1, fmt, vl);
    buffer[sizeof(buffer) - 1] = '\0';
    va_end(vl);

    if (printed > 0) {
        bm_write(STDERR_FILENO, buffer, (size_t) printed, &written);
    }
}

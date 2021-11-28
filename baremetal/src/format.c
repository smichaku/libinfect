#include <baremetal/format.h>

#include <stddef.h>
#include <stdarg.h>

#include <printf.h>

int bm_snprintf(char *str, size_t size, const char *format, ...)
{
    va_list vl;
    int printed;

    va_start(vl, format);
    printed = vsnprintf_(str, size, format, vl);
    va_end(vl);

    return printed;
}

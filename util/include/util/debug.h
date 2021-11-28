#pragma once

#ifdef NDEBUG
#define debug_printf(...)
#else
/* clang-format off */
__attribute__ ((format (printf, 1, 2)))
void debug_printf(const char *fmt, ...);
/* clang-format on */
#endif

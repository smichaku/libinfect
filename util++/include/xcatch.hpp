#include <cstdio>
#include <unistd.h>

#include <catch.hpp>

#include <util/common.h>
#include <util/xerror.h>

#if __has_include(<execinfo.h>)
#include <execinfo.h>

static inline void print_xerror_backtrace()
{
    int err;
    void *frames[XERROR_BACKTRACE_MAXLEN];
    unsigned int count;

    err = xerror_backtrace(frames, ARRAY_SIZE(frames), &count);
    if (err == 0) {
        fprintf(stderr, "xerror backtrace:\n");
        backtrace_symbols_fd(frames, count, STDERR_FILENO);
    }
}
#else
static inline void print_xerror_backtrace()
{
}
#endif

#define REQUIRE_NO_XERR(err)                                                   \
    {                                                                          \
        if (err != 0) {                                                        \
            print_xerror_backtrace();                                          \
        }                                                                      \
                                                                               \
        REQUIRE(err == 0);                                                     \
    }

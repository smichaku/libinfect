#pragma once

#include <stddef.h>
#include <errno.h>
#include <sys/user.h>

#define KB(x) ((size_t) (x) << 10)
#define MB(x) ((size_t) (x) << 20)
#define GB(x) ((size_t) (x) << 30)

#define PAGE_ALIGN(x)    (((size_t) (x)) & ~((pagesize()) - 1))
#define PAGE_ALIGN_UP(x) ((((size_t) (x)) + pagesize() - 1) & -pagesize())
#define PAGE_OFFSET(x)   (((size_t) (x)) & ((pagesize()) - 1))

#define UNUSED(x) (void) (x);

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define MIN(a, b)                                                              \
    (__extension__({                                                           \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        _a < _b ? _a : _b;                                                     \
    }))

#define MAX(a, b)                                                              \
    (__extension__({                                                           \
        __typeof__(a) _a = (a);                                                \
        __typeof__(b) _b = (b);                                                \
        _a > _b ? _a : _b;                                                     \
    }))

/* This macro should typically be used for any syscall which may return with an
 * EINTR error. */
#define RETRY_SYSCALL(expr)                                                    \
    (__extension__({                                                           \
        long int __res;                                                        \
        do                                                                     \
            __res = (long int) (expr);                                         \
        while (__res == -1 && errno == EINTR);                                 \
        __res;                                                                 \
    }))

/* Get the system's page size in bytes. */
size_t pagesize(void);

/* Poor man's cast-away-const in C. */
static inline void *__remove_const_voidp(const void *in)
{
    union {
        void *vp;
        const void *cvp;
    } u;

    u.cvp = in;
    return u.vp;
}

#define REMOVE_CONST(type, ptr) ((type) __remove_const_voidp(ptr))

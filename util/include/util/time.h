#pragma once

#include <stdint.h>
#include <time.h>

/*
 * Time conversion functions.
 */
static inline uint64_t s2ms(uint64_t s)
{
    return s * 1000;
}

static inline uint64_t s2us(uint64_t s)
{
    return s * 1000000;
}

static inline uint64_t s2ns(uint64_t s)
{
    return s * 1000000000;
}

static inline uint64_t ms2s(uint64_t ms)
{
    return ms / 1000;
}

static inline uint64_t ms2us(uint64_t ms)
{
    return ms * 1000;
}

static inline uint64_t ms2ns(uint64_t ms)
{
    return ms * 1000000;
}

static inline uint64_t us2s(uint64_t us)
{
    return us / 1000000;
}

static inline uint64_t us2ms(uint64_t us)
{
    return us / 1000;
}

static inline uint64_t us2ns(uint64_t us)
{
    return us * 1000;
}

static inline uint64_t ns2s(uint64_t ns)
{
    return ns / 1000000000;
}

static inline uint64_t ns2ms(uint64_t ns)
{
    return ns / 1000000;
}

static inline uint64_t ns2us(uint64_t ns)
{
    return ns / 1000;
}

/* Compare two struct timespec objects. */
static inline int timespec_cmp(
    const struct timespec *lhs, const struct timespec *rhs
)
{
    uint64_t lhs_ns = lhs->tv_nsec + s2ns(lhs->tv_sec);
    uint64_t rhs_ns = rhs->tv_nsec + s2ns(rhs->tv_sec);

    return lhs_ns < rhs_ns ? -1 : (lhs_ns > rhs_ns ? 1 : 0);
}

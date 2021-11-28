#include <baremetal/atomic.h>

#include <stdint.h>
#include <stdbool.h>

uint32_t bm_atomic32_cmpxchg(
    volatile uint32_t *var, uint32_t expected, uint32_t desired
)
{
    __atomic_compare_exchange(
        var, &expected, &desired, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST
    );

    return expected;
}

uint32_t bm_atomic32_dec(volatile uint32_t *var)
{
    return __atomic_fetch_sub(var, 1, __ATOMIC_SEQ_CST);
}

void bm_atomic32_load(volatile uint32_t *var, uint32_t *val)
{
    __atomic_load(var, val, __ATOMIC_SEQ_CST);
}

void bm_atomic32_store(volatile uint32_t *var, uint32_t val)
{
    __atomic_store(var, &val, __ATOMIC_SEQ_CST);
}

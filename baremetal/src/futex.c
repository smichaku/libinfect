#include <baremetal/futex.h>

#include <stdint.h>
#include <sys/syscall.h>
#include <linux/futex.h>

#include <syscall.h>

int bm_futex_wait(volatile uint32_t *var, uint32_t expected)
{
    long r;

    r = __syscall(SYS_futex, var, FUTEX_WAIT, expected, 0, 0, 0);
    return __syscall_err(r);
}

int bm_futex_wake(volatile uint32_t *var, uint32_t n)
{
    long r;

    r = __syscall(SYS_futex, var, FUTEX_WAKE, n, 0, 0, 0);
    return __syscall_err(r);
}

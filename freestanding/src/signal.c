#include <freestanding/signal.h>

#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <sys/syscall.h>

#include <freestanding/compiler.h>
#include <freestanding/thread.h>
#include <syscall.h>

static const unsigned long app_mask[] = {
#if ULONG_MAX == 0xffffffff
#if _NSIG == 65
    0x7fffffff, 0xfffffffc
#else
    0x7fffffff, 0xfffffffc, -1UL, -1UL
#endif
#else
#if _NSIG == 65
    0xfffffffc7fffffff
#else
    0xfffffffc7fffffff, -1UL
#endif
#endif
};

static void block_app_sigs(void *set)
{
    __syscall(SYS_rt_sigprocmask, SIG_BLOCK, &app_mask, set, _NSIG / 8);
}

static void restore_sigs(void *set)
{
    __syscall(SYS_rt_sigprocmask, SIG_SETMASK, set, 0, _NSIG / 8);
}

int fs_raise(int sig)
{
    long r;
    sigset_t set;

    block_app_sigs(&set);
    r = __syscall(SYS_tkill, fs_gettid(), sig);
    restore_sigs(&set);

    return __syscall_err(r);
}

weak_alias(fs_raise, raise);

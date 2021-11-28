#include <baremetal/exit.h>

#include <sys/syscall.h>

#include <syscall.h>

_Noreturn void bm_exit(int status)
{
    for (;;) {
        __syscall(SYS_exit, status);
    }
}

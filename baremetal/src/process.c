#include <baremetal/process.h>

#include <sys/types.h>

#include <syscall.h>

pid_t bm_getpid(void)
{
    return __syscall(SYS_getpid);
}

#include <freestanding/process.h>

#include <sys/types.h>

#include <syscall.h>

pid_t fs_getpid(void)
{
    return __syscall(SYS_getpid);
}

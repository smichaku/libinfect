#include <freestanding/exit.h>

#include <sys/syscall.h>

#include <syscall.h>

_Noreturn void fs_exit(int status)
{
    for (;;) {
        __syscall(SYS_exit, status);
    }
}

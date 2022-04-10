#include <freestanding/file.h>

#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <syscall.h>

int fs_open(const char *filename, int flags, mode_t mode, int *fd)
{
    int err;
    long r;

    if (flags & O_CLOEXEC) {
        return -EINVAL;
    }

    r = __syscall(SYS_open, filename, flags, mode);
    err = __syscall_err(r);
    if (err != 0) {
        return err;
    }

    *fd = r;

    return 0;
}

int fs_close(int fd)
{
    return __syscall_err(__syscall(SYS_close, fd));
}

int fs_dup(int oldfd, int *newfd)
{
    int err;
    long r;

    r = __syscall(SYS_dup, oldfd);
    err = __syscall_err(r);
    if (err != 0) {
        return err;
    }

    *newfd = (int) r;

    return 0;
}

int fs_dup2(int oldfd, int newfd)
{
    return __syscall_err(__syscall(SYS_dup2, oldfd, newfd));
}

int fs_read(int fd, void *buf, size_t count, size_t *rd)
{
    int err;
    long r;

    r = __syscall(SYS_read, fd, buf, count);
    err = __syscall_err(r);
    if (err != 0) {
        return err;
    }

    *rd = r;

    return 0;
}

int fs_write(int fd, const void *buf, size_t count, size_t *wt)
{
    int err;
    long r;

    r = __syscall(SYS_write, fd, buf, count);
    err = __syscall_err(r);
    if (err != 0) {
        return err;
    }

    *wt = r;

    return 0;
}

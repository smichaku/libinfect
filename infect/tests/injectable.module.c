#include <stddef.h>
#include <stdarg.h>
#include <fcntl.h>

#include <freestanding/debug.h>
#include <freestanding/file.h>

#define debug_printf fs_debug_printf

int open(const char *filename, int flags, ...)
{
    int err;
    int fd;
    mode_t mode;

    mode = 0;
    if (flags & O_CREAT) {
        va_list vl;

        va_start(vl, flags);
        mode = va_arg(vl, mode_t);
        va_end(vl);
    }

    err = fs_open(filename, flags, mode, &fd);
    return err < 0 ? -1 : fd;
}

int close(int fd)
{
    int err;

    err = fs_close(fd);
    return err < 0 ? -1 : 0;
}

ssize_t write(int fd, const void *buf, size_t count)
{
    int err;
    size_t written;

    err = fs_write(fd, buf, count, &written);
    return err < 0 ? -1 : (ssize_t) written;
}

#include "injectable.c"

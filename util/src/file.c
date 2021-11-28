#include <util/file.h>

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

#include <util/common.h>
#include <util/xerror.h>

int filesize(const char *filename, size_t *size)
{
    int err;
    struct stat st;

    if (filename == NULL || size == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = stat(filename, &st);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    *size = st.st_size;
    return 0;
}

int readall(int fd, void *buf, size_t size, size_t *read_bytes)
{
    size_t total_read;

    if (fd < 0 || buf == NULL || read_bytes == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    total_read = 0;
    while (total_read < size) {
        ssize_t r;

        r = RETRY_SYSCALL(read(fd, (char *) buf + total_read, size - total_read)
        );
        if (r == -1) {
            return xerror(XECLibc, errno);
        }
        if (r == 0) {
            break;
        }

        total_read += (size_t) r;
    }

    *read_bytes = total_read;

    return 0;
}

int read_file(const char *path, void *buf, size_t bufsize, size_t *bytes_read)
{
    int err, ret = 0;
    int fd = -1;

    if (path == NULL || buf == NULL || bytes_read == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        ret = xerror(XECLibc, errno);
        goto out;
    }

    err = readall(fd, buf, bufsize, bytes_read);
    if (err != 0) {
        ret = err;
        goto out;
    }

out:
    if (fd != -1) {
        close(fd);
    }

    return ret;
}

int writeall(int fd, const void *buf, size_t size)
{
    size_t total_written;

    if (fd < 0 || buf == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    total_written = 0;
    while (total_written < size) {
        ssize_t w;

        w = RETRY_SYSCALL(
            write(fd, (const char *) buf + total_written, size - total_written)
        );
        if (w == -1) {
            return xerror(XECLibc, errno);
        }

        total_written += (size_t) w;
    }

    return 0;
}

int write_file(const char *path, mode_t mode, const void *buf, size_t count)
{
    int err, ret = 0;
    int fd = -1;

    if (path == NULL || buf == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, mode);
    if (fd == -1) {
        ret = xerror(XECLibc, errno);
        goto out;
    }

    err = writeall(fd, buf, count);
    if (err != 0) {
        ret = err;
        goto out;
    }

out:
    if (fd != -1) {
        close(fd);
    }

    return ret;
}

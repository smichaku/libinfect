#include <freestanding/mman.h>

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <fcntl.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <freestanding/file.h>

#include <syscall.h>

#define MAX_AUXV 128

int fs_pagesize(size_t *size)
{
    static ssize_t cache = -1;

    int err, ret = 0;
    int fd = -1;
    size_t rd;
    unsigned long auxv[MAX_AUXV];
    unsigned int i, n;
    ssize_t pgsize;

    if (size == NULL) {
        return -EINVAL;
    }

    pgsize = __atomic_load_n(&cache, __ATOMIC_RELAXED);
    if (pgsize != -1) {
        *size = pgsize;
        return 0;
    }

    err = fs_open("/proc/self/auxv", O_RDONLY, 0, &fd);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = fs_read(fd, auxv, sizeof(auxv), &rd);
    if (err != 0) {
        ret = err;
        goto out;
    }

    n = rd / (2 * sizeof(unsigned long));
    for (i = 0; i < n; i++) {
        if (auxv[i] == AT_PAGESZ) {
            *size = pgsize = auxv[i + 1];
            __atomic_store_n(&cache, pgsize, __ATOMIC_RELAXED);
            break;
        }
    }
    if (i == n) {
        ret = -ENOENT;
    }

out:
    if (fd != -1) {
        fs_close(fd);
    }

    return ret;
}

int fs_mmap(
    void *start, size_t len, int prot, int flags, int fd, off_t off, void **map
)
{
    int err;
    long r;
    size_t pgsize;

    err = fs_pagesize(&pgsize);
    if (err != 0) {
        return err;
    }

#ifdef SYS_mmap2
    r = __syscall(SYS_mmap2, start, len, prot, flags, fd, off / pgsize);
#else
    r = __syscall(SYS_mmap, start, len, prot, flags, fd, off);
#endif
    err = __syscall_err(r);
    if (err != 0) {
        return err;
    }

    *map = (void *) r;
    return 0;
}

int fs_munmap(void *start, size_t len)
{
    return __syscall_err(__syscall(SYS_munmap, start, len));
}

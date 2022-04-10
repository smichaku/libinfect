#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int fs_pagesize(size_t *size);

int fs_mmap(
    void *start, size_t len, int prot, int flags, int fd, off_t off, void **map
);

int fs_munmap(void *start, size_t len);

#ifdef __cplusplus
}
#endif

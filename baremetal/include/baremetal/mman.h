#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

int bm_pagesize(size_t *size);

int bm_mmap(
    void *start, size_t len, int prot, int flags, int fd, off_t off, void **map
);

int bm_munmap(void *start, size_t len);

#ifdef __cplusplus
}
#endif

#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/user.h>

#include <util/common.h>

static inline void *encode_remote_handle(uintptr_t addr, size_t size)
{
    size_t npages = size / pagesize();

    return (void *) (addr | npages);
}

static inline void decode_remote_handle(
    void *handle, uintptr_t *addr, size_t *size
)
{
    const size_t PageMask = pagesize() - 1;

    *addr = (size_t) handle & ~PageMask;
    *size = ((size_t) handle & PageMask) * pagesize();
}

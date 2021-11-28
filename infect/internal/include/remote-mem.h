#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <remote.h>

int remote_ptrace_read(
    struct remote *remote, uintptr_t address, void *buf, size_t bufsize
);

int remote_ptrace_write(
    struct remote *remote, uintptr_t address, const void *buf, size_t bufsize
);

int remote_mmap(
    struct remote *remote,
    uintptr_t addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset,
    uintptr_t *map
);

int remote_munmap(struct remote *remote, uintptr_t addr, size_t length);

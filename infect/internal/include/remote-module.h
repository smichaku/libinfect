#pragma once

#include <stddef.h>
#include <stdint.h>

#include <util/elf.h>
#include <remote.h>

#define MODULE_BLOB(module) extern uint8_t module##_start, module##_end

#define MODULE_ADDRESS(module) ((void *) &module##_start)
#define MODULE_SIZE(module)    ((size_t) (&module##_end - &module##_start))

int remote_load_module(struct remote *remote, elf_t elf, void **handle);

int remote_unload_module(struct remote *remote, void *handle);

uintptr_t remote_module_base(struct remote *remote, void *handle);

int remote_module_resolve(
    struct remote *remote,
    void *handle,
    elf_t elf,
    const char *symbol,
    uintptr_t *addr
);

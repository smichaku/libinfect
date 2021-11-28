#pragma once

#include <stdint.h>
#include <sys/types.h>

int resolve_remote_symbol(
    pid_t pid, const char *lib, const char *sym, uintptr_t *value
);

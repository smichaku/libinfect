#pragma once

#include <remote.h>

struct dlfcn {
    uintptr_t dlopen;
    uintptr_t dlclose;
    uintptr_t dlerror;
};

int resolve_remote_dlfcn(pid_t pid, struct dlfcn *dlfcn);

int remote_dlopen(
    struct remote *remote, const char *filename, int flags, void **handle
);

int remote_dlclose(struct remote *remote, void *handle);

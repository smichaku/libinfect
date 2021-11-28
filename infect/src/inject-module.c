#include "remote-module.h"
#include "util/xerror.h"
#include <infect/inject.h>

#include <stddef.h>

#include <util/elf.h>

#include <remote.h>

int inject_module(pid_t pid, const char *path, void **handle)
{
    int err, ret = 0;
    elf_t elf = NULL;
    struct remote *remote = NULL;

    if (pid == 0 || path == NULL || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = elf_open(path, &elf);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_load_module(remote, elf, handle);
    if (err != 0) {
        ret = err;
        goto out;
    }

out:
    if (remote != NULL) {
        remote_close(remote);
    }

    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

int eject_module(pid_t pid, void *handle)
{
    int err, ret = 0;
    struct remote *remote = NULL;

    if (pid == 0 || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_unload_module(remote, handle);
    if (err != 0) {
        ret = err;
        goto out;
    }

out:
    if (remote != NULL) {
        remote_close(remote);
    }

    return ret;
}

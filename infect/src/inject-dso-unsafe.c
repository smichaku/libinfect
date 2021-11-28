#include <infect/inject.h>

#include <dlfcn.h>

#include <util/xerror.h>

#include <remote-dlfcn.h>

int inject_dso_unsafe(pid_t pid, const char *lib, void **handle)
{
    int err, ret = 0;
    struct remote *remote = NULL;

    if (pid == 0 || lib == NULL || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_dlopen(remote, lib, RTLD_NOW, handle);
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

int eject_dso_unsafe(pid_t pid, void *handle)
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

    err = remote_dlclose(remote, handle);
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

#include <remote-dlfcn.h>

#include <stdint.h>
#include <string.h>

#include <util/common.h>
#include <util/xerror.h>
#include <util/debug.h>

#include <remote.h>
#include <remote-stdlib.h>
#include <resolve.h>

int resolve_remote_dlfcn(pid_t pid, struct dlfcn *dlfcn)
{
    int err;

    err = resolve_remote_symbol(pid, "libdl", "dlopen", &dlfcn->dlopen);
    if (err == XERR_NOT_FOUND) {
        err = resolve_remote_symbol(pid, "libc", "dlopen", &dlfcn->dlopen);
        if (err == XERR_NOT_FOUND) {
            err = resolve_remote_symbol(
                pid, "libc", "__libc_dlopen_mode", &dlfcn->dlopen
            );
        }
    }
    if (err != 0) {
        return err;
    }

    err = resolve_remote_symbol(pid, "libdl", "dlclose", &dlfcn->dlclose);
    if (err == XERR_NOT_FOUND) {
        err = resolve_remote_symbol(pid, "libc", "dlclose", &dlfcn->dlclose);
        if (err == XERR_NOT_FOUND) {
            err = resolve_remote_symbol(
                pid, "libc", "__libc_dlclose", &dlfcn->dlclose
            );
        }
    }
    if (err != 0) {
        return err;
    }

    err = resolve_remote_symbol(pid, "libdl", "dlerror", &dlfcn->dlerror);
    if (err != 0) {
        if (err == XERR_NOT_FOUND) {
            dlfcn->dlerror = 0;
        }
    }

    return 0;
}

int remote_dlopen(
    struct remote *remote, const char *filename, int flags, void **handle
)
{
    int err, ret = 0;
    struct dlfcn dlfcn;
    size_t size;
    uintptr_t remote_filename = 0;
    int written;
    struct remote_arg args[2];
    struct remote_arg rv;

    debug_printf(
        "remote_dlopen(pid=%d, filename=%s, flags=%x)\n",
        remote->pid,
        filename,
        flags
    );

    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    err = resolve_remote_dlfcn(remote->pid, &dlfcn);
    if (err != 0) {
        ret = err;
        goto out;
    }

    size = strlen(filename) + 1;
    err = remote_malloc(remote, size, &remote_filename);
    if (err != 0) {
        ret = err;
        goto out;
    }
    if (remote_filename == 0) {
        ret = XERR(INSUFFICIENT_MEMORY);
        goto out;
    }

    written = remote_write(remote, remote_filename, filename, size);
    if (written != 0) {
        ret = err;
        goto out;
    }

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = remote_filename;
    args[1].type = RemoteArgInt;
    args[1].arg.v_int = flags;
    rv.type = RemoteArgPointer;
    err = remote_call(remote, dlfcn.dlopen, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        ret = err;
        goto out;
    }
    if (rv.arg.v_pointer == 0) {
        ret = XERR(UNEXPECTED_ERROR);
        goto out;
    }

    *handle = (void *) rv.arg.v_pointer;

out:
    if (remote_filename != 0) {
        remote_free(remote, remote_filename);
    }

    return ret;
}

int remote_dlclose(struct remote *remote, void *handle)
{
    int err;
    struct dlfcn dlfcn;
    struct remote_arg args[1];
    struct remote_arg rv;

    debug_printf("remote_dlclose(pid=%d, handle=%p)\n", remote->pid, handle);

    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    err = resolve_remote_dlfcn(remote->pid, &dlfcn);
    if (err != 0) {
        return err;
    }

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = (uintptr_t) handle;
    rv.type = RemoteArgInt;
    err = remote_call(remote, dlfcn.dlclose, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        return err;
    }
    if (rv.arg.v_int != 0) {
        return XERR(UNEXPECTED_ERROR);
    }

    return 0;
}

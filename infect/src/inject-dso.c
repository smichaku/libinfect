#include <infect/inject.h>

#include <stddef.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <dlfcn.h>

#include <util/common.h>
#include <util/debug.h>
#include <util/xerror.h>
#include <util/time.h>
#include <util/elf.h>

#include <config.h>
#include <remote.h>
#include <remote-mem.h>
#include <remote-dlfcn.h>
#include <remote-module.h>
#include <module-promise.h>
#include <resolve.h>

MODULE_BLOB(async_dlfcn);

static const struct timespec PromiseTimeout = { 5, 0 };

static int wait_on_promise(
    struct remote *remote,
    void *module_handle,
    elf_t elf,
    uintptr_t remote_promise,
    const struct timespec *timeout
)
{
    int err;
    uintptr_t function;
    struct remote_arg args[1], rv;
    struct timespec now, due;

    err = remote_module_resolve(
        remote, module_handle, elf, "async_dlfcn_ready", &function
    );
    if (err != 0) {
        return err;
    }

    clock_gettime(CLOCK_MONOTONIC, &now);
    due.tv_sec = now.tv_sec + timeout->tv_sec;
    due.tv_nsec = now.tv_nsec + timeout->tv_nsec;

    do {
        bool ready;

        args[0].type = RemoteArgPointer;
        args[0].arg.v_pointer = remote_promise;
        rv.type = RemoteArgBool;
        err = remote_call(remote, function, args, ARRAY_SIZE(args), &rv);
        if (err != 0) {
            return err;
        }

        ready = rv.arg.v_bool;
        debug_printf("async_dlfcn_ready() -> %d\n", ready);
        if (ready) {
            break;
        }

        err = remote_detach(remote);
        if (err != 0) {
            return err;
        }

        usleep(CONFIG_PROMISE_POLL_PERIOD_US);

        err = remote_attach(remote);
        if (err != 0) {
            return err;
        }
    } while (timespec_cmp(&now, &due) < 0);

    return 0;
}

static int do_dlfcn_init(struct remote *remote, void *module_handle, elf_t elf)
{
    int err;
    struct dlfcn dlfcn;
    uintptr_t function;
    struct remote_arg args[3];

    debug_printf("do_dlfcn_init(module_handle=%p)\n", module_handle);

    err = resolve_remote_dlfcn(remote->pid, &dlfcn);
    if (err != 0) {
        return err;
    }

    err = remote_module_resolve(
        remote, module_handle, elf, "async_dlfcn_init", &function
    );
    if (err != 0) {
        return err;
    }

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = dlfcn.dlopen;
    args[1].type = RemoteArgPointer;
    args[1].arg.v_pointer = dlfcn.dlclose;
    args[2].type = RemoteArgPointer;
    args[2].arg.v_pointer = dlfcn.dlerror;
    err = remote_call(remote, function, args, ARRAY_SIZE(args), NULL);
    if (err != 0) {
        return err;
    }

    return 0;
}

static int do_dlopen(
    struct remote *remote,
    void *module_handle,
    elf_t elf,
    const char *path,
    int flags,
    void **handle
)
{
    struct args_buffer {
        struct module_promise promise;
        char path[1];
    };

    int err, ret = 0;
    uintptr_t function;
    size_t path_len, map_size = 0;
    uintptr_t map = 0, remote_path, remote_promise, remote_handle;
    struct remote_arg args[3], rv;

    debug_printf(
        "do_dlopen(module_handle=%p, path=%s, flags=%x)\n",
        module_handle,
        path,
        flags
    );

    err = remote_module_resolve(
        remote, module_handle, elf, "async_dlopen", &function
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    path_len = strlen(path);
    map_size = PAGE_ALIGN_UP(sizeof(struct args_buffer) + path_len + 1);
    err = remote_mmap(
        remote,
        0,
        map_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0,
        &map
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    remote_path = map + offsetof(struct args_buffer, path);
    remote_promise = map + offsetof(struct args_buffer, promise);
    remote_handle = remote_promise + offsetof(struct module_promise, value);

    err = remote_write(remote, remote_path, path, path_len + 1);
    if (err != 0) {
        ret = err;
        goto out;
    }

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = remote_path;
    args[1].type = RemoteArgInt;
    args[1].arg.v_int = flags;
    args[2].type = RemoteArgPointer;
    args[2].arg.v_pointer = remote_promise;
    rv.type = RemoteArgInt;
    err = remote_call(remote, function, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        ret = err;
        goto out;
    }
    if (rv.arg.v_int != 0) {
        ret = xerror(XECLibc, -rv.arg.v_int);
        goto out;
    }

    err = wait_on_promise(
        remote, module_handle, elf, remote_promise, &PromiseTimeout
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_read(remote, remote_handle, handle, sizeof(void *));
    if (err != 0) {
        ret = err;
        goto out;
    }

out:
    if (map != 0) {
        remote_munmap(remote, map, map_size);
    }

    return ret;
}

static int do_dlclose(
    struct remote *remote,
    void *module_handle,
    elf_t elf,
    void *handle,
    int *retval
)
{
    struct args_buffer {
        struct module_promise promise;
    };

    int err, ret = 0;
    uintptr_t function;
    uintptr_t map = 0, remote_promise, remote_retval;
    size_t map_size;
    struct remote_arg args[2], rv;
    void *value;

    err = remote_module_resolve(
        remote, module_handle, elf, "async_dlclose", &function
    );
    if (err != 0) {
        return err;
    }

    map_size = PAGE_ALIGN_UP(sizeof(struct args_buffer));
    err = remote_mmap(
        remote,
        0,
        map_size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0,
        &map
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    remote_promise = map + offsetof(struct args_buffer, promise);
    remote_retval = remote_promise + offsetof(struct module_promise, value);

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = (uintptr_t) handle;
    args[1].type = RemoteArgPointer;
    args[1].arg.v_pointer = remote_promise;
    rv.type = RemoteArgInt;
    err = remote_call(remote, function, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        return err;
    }
    if (rv.arg.v_int != 0) {
        ret = xerror(XECLibc, rv.arg.v_int);
        goto out;
    }

    err = wait_on_promise(
        remote, module_handle, elf, remote_promise, &PromiseTimeout
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_read(remote, remote_retval, &value, sizeof(void *));
    if (err != 0) {
        ret = err;
        goto out;
    }

    *retval = (int) (uintptr_t) value;

out:
    if (map != 0) {
        remote_munmap(remote, map, map_size);
    }

    return ret;
}

int inject_dso(pid_t pid, const char *lib, void **handle)
{
    int err, ret = 0;
    elf_t elf = NULL;
    struct remote *remote = NULL;
    void *module_handle = NULL;
    void *h;

    if (pid == 0 || lib == NULL || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = elf_open_mem(
        MODULE_ADDRESS(async_dlfcn), MODULE_SIZE(async_dlfcn), &elf
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_load_module(remote, elf, &module_handle);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = do_dlfcn_init(remote, module_handle, elf);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = do_dlopen(remote, module_handle, elf, lib, RTLD_NOW, &h);
    if (err != 0) {
        ret = err;
        goto out;
    }
    if (h == NULL) {
        ret = XERR(UNEXPECTED_ERROR);
        goto out;
    }

    *handle = h;

out:
    if (module_handle != NULL && !remote->terminated) {
        remote_unload_module(remote, module_handle);
    }

    if (remote != NULL) {
        remote_close(remote);
    }

    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

int eject_dso(pid_t pid, void *handle)
{
    int err, ret = 0;
    elf_t elf = NULL;
    struct remote *remote = NULL;
    void *module_handle = NULL;
    int rv = 0;

    if (pid == 0 || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = elf_open_mem(
        MODULE_ADDRESS(async_dlfcn), MODULE_SIZE(async_dlfcn), &elf
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_load_module(remote, elf, &module_handle);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = do_dlfcn_init(remote, module_handle, elf);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = do_dlclose(remote, module_handle, elf, handle, &rv);
    if (err != 0) {
        ret = err;
        goto out;
    }
    if (rv != 0) {
        ret = XERR(UNEXPECTED_ERROR);
        goto out;
    }

out:
    if (module_handle != NULL) {
        remote_unload_module(remote, module_handle);
    }

    if (remote != NULL) {
        remote_close(remote);
    }

    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

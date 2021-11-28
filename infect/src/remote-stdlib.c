#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>

#include <util/common.h>
#include <util/xerror.h>
#include <util/debug.h>

#include <remote.h>
#include <resolve.h>

int remote_malloc(struct remote *remote, size_t size, uintptr_t *ptr)
{
    int err;
    uintptr_t function;
    struct remote_arg args[1];
    struct remote_arg rv;

    debug_printf("remote_malloc(pid=%d, size=%zu)\n", remote->pid, size);

    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    err = resolve_remote_symbol(remote->pid, "libc", "malloc", &function);
    if (err != 0) {
        return err;
    }

    args[0].type = RemoteArgSize;
    args[0].arg.v_size = size;
    rv.type = RemoteArgPointer;
    err = remote_call(remote, function, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        return err;
    }

    *ptr = rv.arg.v_pointer;

    debug_printf("remote_malloc() -> 0x%" PRIxPTR "\n", *ptr);

    return 0;
}

int remote_free(struct remote *remote, uintptr_t ptr)
{
    int err;
    uintptr_t function;
    struct remote_arg args[1];

    debug_printf("remote_free(pid=%d, ptr=0x%" PRIxPTR ")\n", remote->pid, ptr);

    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    err = resolve_remote_symbol(remote->pid, "libc", "free", &function);
    if (err != 0) {
        return err;
    }

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = ptr;
    err = remote_call(remote, function, args, ARRAY_SIZE(args), NULL);
    if (err != 0) {
        return err;
    }

    return 0;
}

#include <infect/inject.h>

#include <stddef.h>
#include <stdint.h>
#include <sys/mman.h>

#include <util/common.h>
#include <util/xerror.h>

#include <remote.h>
#include <remote-mem.h>
#include <remote-handle.h>

int inject_code(
    pid_t pid, const void *code, size_t code_size, size_t entry, void **handle
)
{
    int err, ret = 0;
    size_t code_size_aligned = PAGE_ALIGN_UP(code_size);
    struct remote *remote = NULL;
    uintptr_t remote_code = 0;

    if (pid == 0 || code == NULL || code_size == 0 || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_mmap(
        remote,
        0,
        code_size_aligned,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_SHARED | MAP_ANONYMOUS,
        -1,
        0,
        &remote_code
    );
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_write(remote, remote_code, code, code_size);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_call(remote, remote_code + entry, NULL, 0, NULL);
    if (err != 0) {
        ret = err;
        goto out;
    }

    *handle = encode_remote_handle(remote_code, code_size_aligned);

out:
    if (ret != 0) {
        if (remote_code != 0) {
            remote_munmap(remote, remote_code, code_size_aligned);
        }
    }

    if (remote != NULL) {
        remote_close(remote);
    }

    return ret;
}

int eject_code(pid_t pid, void *handle)
{
    int err, ret = 0;
    uintptr_t remote_code;
    size_t code_size;
    struct remote *remote = NULL;

    if (pid == 0 || handle == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    decode_remote_handle(handle, &remote_code, &code_size);

    err = remote_open(pid, &remote, 0);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = remote_munmap(remote, remote_code, code_size);
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

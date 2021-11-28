#include <remote-mem.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>

#include <util/common.h>
#include <util/debug.h>
#include <util/syscall.h>
#include <util/xerror.h>
#include <util/syscall.h>

#include <remote.h>

typedef enum
{
    REMOTE_MEMOP_PEEK,
    REMOTE_MEMOP_POKE,
} remote_memop_t;

typedef union {
    long l;
    uint8_t raw[sizeof(long)];
} memlong_t;

static int peek(struct remote *remote, uintptr_t address, void *ptr)
{
    long data;

    errno = 0;
    data = ptrace(PTRACE_PEEKDATA, remote->pid, address, 0);
    if (errno != 0) {
        return xerror(XECLibc, errno);
    }

    *((long *) ptr) = data;

    return 0;
}

static int poke(struct remote *remote, uintptr_t address, void *ptr)
{
    long data;

    data = *((long *) ptr);

    errno = 0;
    data = ptrace(PTRACE_POKEDATA, remote->pid, address, data);
    if (errno != 0) {
        return xerror(XECLibc, errno);
    }

    return 0;
}

static int access_memory(
    struct remote *remote,
    remote_memop_t op,
    uintptr_t address,
    void *buf,
    size_t bufsize
)
{
    int err;
    char *bufptr;
    int (*access)(struct remote *, uintptr_t, void *);

    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    if (bufsize == 0) {
        return 0;
    }

    switch (op) {
    case REMOTE_MEMOP_PEEK:
        access = peek;
        break;
    case REMOTE_MEMOP_POKE:
        access = poke;
        break;
    }

    bufptr = buf;

    /* Copy whole aligned words with a simple store operation without memcpy. */
    while (bufsize >= sizeof(long)) {
        err = access(remote, address, bufptr);
        if (err != 0) {
            return err;
        }

        bufptr += sizeof(long);
        address += sizeof(long);
        bufsize -= sizeof(long);
    }

    /* Copy any remaining bytes with memcpy() in order to stay in bounds. */
    if (bufsize > 0) {
        memlong_t data;

        if (op == REMOTE_MEMOP_POKE) {
            memcpy(data.raw, bufptr, bufsize);
        }

        err = access(remote, address, &data.l);
        if (err != 0) {
            return err;
        }

        if (op == REMOTE_MEMOP_PEEK) {
            memcpy(bufptr, data.raw, bufsize);
        }

        bufptr += bufsize;
        address += bufsize;
        bufsize = 0;
    }

    return 0;
}

int remote_ptrace_read(
    struct remote *remote, uintptr_t address, void *buf, size_t bufsize
)
{
    return access_memory(remote, REMOTE_MEMOP_PEEK, address, buf, bufsize);
}

int remote_ptrace_write(
    struct remote *remote, uintptr_t address, const void *buf, size_t bufsize
)
{
    return access_memory(
        remote, REMOTE_MEMOP_POKE, address, REMOVE_CONST(void *, buf), bufsize
    );
}

int remote_mmap(
    struct remote *remote,
    uintptr_t addr,
    size_t length,
    int prot,
    int flags,
    int fd,
    off_t offset,
    uintptr_t *map
)
{
    int err;
    struct remote_arg args[6];
    struct remote_arg rv;
    long scn;

    debug_printf(
        "remote_mmap(pid=%d, addr=0x%" PRIxPTR
        ", size=%zu, prot=0x%x, flags=0x%x, fd=%d)\n",
        remote->pid,
        addr,
        length,
        prot,
        flags,
        fd
    );

    if (offset % pagesize() > 0) {
        return XERR(INVALID_ARGUMENT);
    }

#ifdef SYS_mmap2
    /* The mmap2() syscall should be given the same arguments except for the
     * offset which is in 4K units instead of bytes (see mmap2(2)). */
    scn = SYS_mmap2;
    offset /= pagesize();
#else
    scn = SYS_mmap;
#endif

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = addr;
    args[1].type = RemoteArgSize;
    args[1].arg.v_size = length;
    args[2].type = RemoteArgInt;
    args[2].arg.v_int = prot;
    args[3].type = RemoteArgInt;
    args[3].arg.v_int = flags;
    args[4].type = RemoteArgInt;
    args[4].arg.v_int = fd;
    args[5].type = RemoteArgLong;
    args[5].arg.v_ulong = offset;
    rv.type = RemoteArgPointer;
    err = remote_syscall(remote, scn, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        return err;
    }
    err = syscall_error(rv.arg.v_ulong);
    if (err != 0) {
        return xerror(XECLibc, err);
    }

    *map = rv.arg.v_pointer;

    return 0;
}

int remote_munmap(struct remote *remote, uintptr_t addr, size_t length)
{
    int err;
    struct remote_arg args[2];
    struct remote_arg rv;

    debug_printf(
        "remote_munmap(pid=%d, addr=0x%" PRIxPTR ", length=%zu)\n",
        remote->pid,
        addr,
        length
    );

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = addr;
    args[1].type = RemoteArgSize;
    args[1].arg.v_size = length;
    rv.type = RemoteArgInt;
    err = remote_syscall(remote, SYS_munmap, args, ARRAY_SIZE(args), &rv);
    if (err != 0) {
        return err;
    }
    err = syscall_error(rv.arg.v_ulong);
    if (err != 0) {
        return xerror(XECLibc, err);
    }

    return 0;
}

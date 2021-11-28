#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_AUXS        64
#define MAX_REGS        32
#define MAX_ENTRY_BYTES 16

struct remote;

typedef enum remote_arg_type
{
    RemoteArgInt,
    RemoteArgUInt,
    RemoteArgLong,
    RemoteArgULong,
    RemoteArgBool,
    RemoteArgPointer,
    RemoteArgSize,
} remote_arg_type_t;

enum remote_flag
{
    RemoteFlagSingleStep = 0x0001
};

struct remote_arg {
    remote_arg_type_t type;

    union {
        int v_int;
        unsigned int v_uint;
        long v_long;
        unsigned long v_ulong;
        bool v_bool;
        uintptr_t v_pointer;
        size_t v_size;
    } arg;
};

struct remote_regs {
    unsigned long r[MAX_REGS];
};

struct remote_arch_ops {
    /* clang-format off */
    void (*set_pattern_regs)(struct remote_regs *regs);

    bool (*check_pattern_regs)(const struct remote_regs *regs);

    int (*call)(
        struct remote *remote,
        struct remote_regs *regs,
        uintptr_t function,
        const struct remote_arg *args,
        unsigned int nargs
    );

    int (*ret)(
        struct remote *remote,
        struct remote_regs *regs,
        struct remote_arg *retval
    );

    int (*syscall)(
        struct remote *remote,
        struct remote_regs *regs,
        long number,
        const struct remote_arg *args,
        unsigned int nargs
    );

    int (*sysret)(
        struct remote *remote,
        struct remote_regs *regs,
        struct remote_arg *retval
    );

    void (*debug_dump)(struct remote *remote, const struct remote_regs *regs);
    /* clang-format on */
};

struct remote {
    pid_t pid;

    int flags;

    unsigned int attached : 1;
    unsigned int terminated : 1;
    unsigned int fault : 1;

    unsigned long auxv[MAX_AUXS];

    uint8_t old_entry[MAX_ENTRY_BYTES];

    const struct remote_arch_ops *arch_ops;

    struct {
        struct remote_regs regs;
    } restore;

    void *arch;
};

int remote_open(pid_t pid, struct remote **remote, int flags);

int remote_close(struct remote *remote);

int remote_attach(struct remote *remote);

int remote_detach(struct remote *remote);

int remote_read(
    struct remote *remote, uintptr_t address, void *data, size_t size
);

int remote_write(
    struct remote *remote, uintptr_t address, const void *data, size_t size
);

int remote_call(
    struct remote *remote,
    uintptr_t function,
    const struct remote_arg *args,
    unsigned int nargs,
    struct remote_arg *retval
);

int remote_syscall(
    struct remote *remote,
    long number,
    const struct remote_arg *args,
    size_t nargs,
    struct remote_arg *rv
);

void remote_debug_dump(struct remote *remote, const char *msg);

int remote_arch_open(struct remote *remote);

int remote_arch_close(struct remote *remote);

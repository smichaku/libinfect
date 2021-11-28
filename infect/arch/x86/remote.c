#include <remote.h>

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <elf.h>
#include <sys/user.h>

#include <util/compiler.h>
#include <util/common.h>
#include <util/debug.h>
#include <util/xerror.h>

/* The definitions here follow the SystemV AMD64 ABI. */

#define regidx(r)                                                              \
    (offsetof(struct user_regs_struct, r) /                                    \
     sizeof(((struct user_regs_struct *) 0)->r))

#define AX    regidx(rax)
#define SP    regidx(rsp)
#define IP    regidx(rip)
#define FLAGS regidx(eflags)

#define PATTERN_SEED 0x1000000000000000LU
#define PATTERN_INC  0x1111111111111111LU

static const unsigned int ParamRegIdx[] = {
    regidx(rdi), regidx(rsi), regidx(rdx), regidx(rcx), regidx(r8), regidx(r9),
};

static const unsigned int SyscallParamRegIdx[] = {
    regidx(rdi), regidx(rsi), regidx(rdx), regidx(r10), regidx(r8), regidx(r9),
};

static const unsigned int PatternRegIdx[] = {
    regidx(rbx), regidx(rbp), regidx(r12), regidx(r13), regidx(r15),
};

/*
 * syscall
 */
static const uint8_t SyscallCode[] = { 0x0f, 0x05 };

/*
 * call %rax
 * ret
 */
static const uint8_t CallStubCode[] = { 0xff, 0xd0, 0xc3 };

static void set_pattern_regs(struct remote_regs *regs)
{
    unsigned int i;
    unsigned long pattern = PATTERN_SEED;

    for (i = 0; i < ARRAY_SIZE(PatternRegIdx); i++) {
        regs->r[PatternRegIdx[i]] = pattern;
        pattern += PATTERN_INC;
    }
}

static bool check_pattern_regs(const struct remote_regs *regs)
{
    unsigned int i;
    unsigned long pattern = PATTERN_SEED;

    for (i = 0; i < ARRAY_SIZE(PatternRegIdx); i++) {
        if (regs->r[PatternRegIdx[i]] != pattern) {
            return false;
        }

        pattern += PATTERN_INC;
    }

    return true;
}

static int call(
    struct remote *remote,
    struct remote_regs *regs,
    uintptr_t function,
    const struct remote_arg *args,
    unsigned int nargs
)
{
    static const unsigned long Zero = 0;

    int err;
    unsigned int i;
    unsigned long sp;
    uintptr_t entry;
    uint8_t old_entry[sizeof(CallStubCode)] = { 0 };

    _Static_assert(
        sizeof(CallStubCode) <= MAX_ENTRY_BYTES,
        "Insufficient space for storing old entry bytes"
    );

    /* XXX: Calling remote functions which take more arguments than can fit in
     * registers is currently not supported. */
    if (nargs > ARRAY_SIZE(ParamRegIdx)) {
        return XERR(TOO_MANY_ARGS);
    }

    for (i = 0; i < nargs; i++) {
        regs->r[ParamRegIdx[i]] = args[i].arg.v_ulong;
    }

    /* SP needs to be 16 bytes aligned according to the SystemV AMD64 ABI. */
    sp = (regs->r[SP] - sizeof(unsigned long)) & ~0xfUL;
    err = remote_write(remote, sp, &Zero, sizeof(Zero));
    if (err != 0) {
        return err;
    }

    entry = remote->auxv[AT_ENTRY];

    err = remote_read(remote, entry, old_entry, sizeof(CallStubCode));
    if (err != 0) {
        return err;
    }

    err = remote_write(remote, entry, CallStubCode, sizeof(CallStubCode));
    if (err != 0) {
        return err;
    }

    regs->r[SP] = sp;
    regs->r[AX] = function;
    regs->r[IP] = entry;

    return 0;
}

static int ret(
    struct remote *remote, struct remote_regs *regs, struct remote_arg *retval
)
{
    int err;
    uintptr_t entry;

    entry = remote->auxv[AT_ENTRY];

    err = remote_write(remote, entry, remote->old_entry, sizeof(CallStubCode));
    if (err != 0) {
        return err;
    }

    regs->r[SP] = remote->restore.regs.r[SP];

    if (retval != NULL) {
        retval->arg.v_ulong = regs->r[AX];
    }

    return 0;
}

static int syscall(
    struct remote *remote,
    struct remote_regs *regs,
    long number,
    const struct remote_arg *args,
    unsigned int nargs
)
{
    int err;
    unsigned int i;
    uintptr_t entry;

    _Static_assert(
        sizeof(SyscallCode) <= MAX_ENTRY_BYTES,
        "Insufficient space for storing old entry bytes"
    );

    /* Are there syscalls with more than 6 arguments? Not supported... */
    if (nargs > 6) {
        return XERR(TOO_MANY_ARGS);
    }

    for (i = 0; i < nargs; i++) {
        regs->r[SyscallParamRegIdx[i]] = args[i].arg.v_ulong;
    }

    entry = remote->auxv[AT_ENTRY];

    err = remote_write(remote, entry, SyscallCode, sizeof(SyscallCode));
    if (err != 0) {
        return err;
    }

    regs->r[AX] = number;
    regs->r[IP] = entry;

    return 0;
}

static int sysret(
    unused struct remote *remote,
    struct remote_regs *regs,
    struct remote_arg *retval
)
{
    if (retval != NULL) {
        retval->arg.v_ulong = regs->r[AX];
    }

    return 0;
}

static void debug_dump(struct remote *remote, const struct remote_regs *regs)
{
    int err;
    const struct user_regs_struct *r = (const struct user_regs_struct *) regs;
    uint8_t code[128];
    unsigned long stack[32];
    unsigned int i;

    debug_printf(
        "RIP=%016llx, RSP=%016llx, EFLAGS=%08llx\n", r->rip, r->rsp, r->eflags
    );
    debug_printf(
        "RAX=%016llx, RBX=%016llx, RCX=%016llx\n", r->rax, r->rbx, r->rcx
    );
    debug_printf(
        "RDX=%016llx, RSI=%016llx, RDI=%016llx\n", r->rdx, r->rsi, r->rdi
    );
    debug_printf(
        "RBP=%016llx, R08=%016llx, R09=%016llx\n", r->rbp, r->r8, r->r9
    );
    debug_printf(
        "R10=%016llx, R11=%016llx, R12=%016llx\n", r->r10, r->r11, r->r12
    );
    debug_printf(
        "R13=%016llx, R14=%016llx, R15=%016llx\n", r->r13, r->r14, r->r15
    );
    debug_printf("FS=%016llx, GS=%016llx\n", r->fs, r->gs);

    err = remote_read(remote, r->rip, code, sizeof(code));
    if (err == 0) {
        debug_printf("Code:\n");
        for (i = 0; i < sizeof(code); i++) {
            if (i % 16 == 0) {
                if (i > 0) {
                    debug_printf("\n");
                }
                debug_printf("%016llx: ", r->rip + i);
            }

            debug_printf("%02hhx ", code[i]);
        }
        debug_printf("\n");
    } else {
        debug_printf("Unable to read code\n");
    }

    err = remote_read(remote, r->rsp, stack, sizeof(stack));
    if (err == 0) {
        debug_printf("Stack:\n");
        for (i = 0; i < ARRAY_SIZE(stack); i++) {
            debug_printf(
                "%016llx: %016lx\n",
                r->rsp + i * sizeof(unsigned long),
                stack[i]
            );
        }
        debug_printf("\n");
    } else {
        debug_printf("Unable to read stack\n");
    }
}

static const struct remote_arch_ops x86_ops = {
    .set_pattern_regs = set_pattern_regs,
    .check_pattern_regs = check_pattern_regs,
    .call = call,
    .ret = ret,
    .syscall = syscall,
    .sysret = sysret,
    .debug_dump = debug_dump,
};

int remote_arch_open(struct remote *remote)
{
    remote->arch_ops = &x86_ops;

    return 0;
}

int remote_arch_close(struct remote *remote)
{
    UNUSED(remote);

    return 0;
}

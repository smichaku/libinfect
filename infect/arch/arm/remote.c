#include <remote.h>

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <elf.h>
#include <sys/types.h>
#include <sys/user.h>

#include <util/compiler.h>
#include <util/common.h>
#include <util/xerror.h>
#include <util/debug.h>

#define FP   11
#define IP   12
#define SP   13
#define LR   14
#define PC   15
#define CPSR 16

#define CPSR_BIT_T ((uint32_t) 0x20)

#define PATTERN_SEED 0x10000000LU
#define PATTERN_INC  0x11111111LU

struct remote_arm {
    unsigned int cpsr_t : 1;
};

static const unsigned int ParamRegIdx[] = { 0, 1, 2, 3 };

static const unsigned int SyscallParamRegIdx[] = { 0, 1, 2, 3, 4, 5 };

static const unsigned int PatternRegIdx[] = { 4, 5, 6, 7, 8, 10, 11 };

/*
 * svc 0
 */
static const uint8_t SVCARM[] = { 0x00, 0x00, 0x00, 0xef };
static const uint8_t SVCTHUMB[] = { 0x00, 0xdf };

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
    int err;
    unsigned int i;
    struct remote_arm *arch = remote->arch;

    for (i = 0; i < MIN(nargs, ARRAY_SIZE(ParamRegIdx)); i++) {
        regs->r[ParamRegIdx[i]] = args[i].arg.v_ulong;
    }

    for (; i < nargs; i++) {
        regs->r[SP] -= sizeof(unsigned long);

        err = remote_write(
            remote, regs->r[SP], &args[i].arg.v_ulong, sizeof(unsigned long)
        );
        if (err != 0) {
            return err;
        }
    }

    regs->r[LR] = 0;
    regs->r[PC] = function;

    arch->cpsr_t = (regs->r[CPSR] & CPSR_BIT_T) ? 1 : 0;
    if (function & 1) {
        regs->r[CPSR] |= CPSR_BIT_T;
    } else {
        regs->r[CPSR] &= ~CPSR_BIT_T;
    }

    return 0;
}

static int ret(
    unused struct remote *remote,
    struct remote_regs *regs,
    struct remote_arg *retval
)
{
    struct remote_arm *arch = remote->arch;

    if (retval != NULL) {
        retval->arg.v_ulong = regs->r[0];
    }

    if (arch->cpsr_t) {
        regs->r[CPSR] |= CPSR_BIT_T;
    } else {
        regs->r[CPSR] &= ~CPSR_BIT_T;
    }
    arch->cpsr_t = 0;

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
    const uint8_t *svc_code;
    size_t svc_size;

    _Static_assert(
        sizeof(SVCARM) < MAX_ENTRY_BYTES && sizeof(SVCTHUMB) < MAX_ENTRY_BYTES,
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

    if (regs->r[CPSR] & CPSR_BIT_T) {
        svc_code = SVCTHUMB;
        svc_size = sizeof(SVCTHUMB);
    } else {
        svc_code = SVCARM;
        svc_size = sizeof(SVCARM);
    }

    err = remote_write(remote, entry, svc_code, svc_size);
    if (err != 0) {
        return err;
    }

    regs->r[7] = number;
    regs->r[PC] = entry;

    return 0;
}

static int sysret(
    unused struct remote *remote,
    struct remote_regs *regs,
    struct remote_arg *retval
)
{
    if (retval != NULL) {
        retval->arg.v_ulong = regs->r[0];
    }

    return 0;
}

static void debug_dump(struct remote *remote, const struct remote_regs *regs)
{
    int err;
    const unsigned long *r = regs->r;
    uint8_t code[128];
    unsigned long stack[32];
    unsigned int i;

    debug_printf("PC=%016lx, SP=%016lx, LR=%016lx\n", r[PC], r[SP], r[LR]);
    debug_printf("R0=%016lx, R1=%016lx, R2=%016lx\n", r[0], r[1], r[2]);
    debug_printf("R3=%016lx, R4=%016lx, R5=%016lx\n", r[3], r[4], r[5]);
    debug_printf("R6=%016lx, R7=%016lx, R8=%016lx\n", r[6], r[7], r[8]);
    debug_printf("R9=%016lx, R10=%016lx, R11=%016lx\n", r[9], r[10], r[11]);
    debug_printf("R12=%016lx\n", r[12]);

    err = remote_read(remote, r[PC], code, sizeof(code));
    if (err == 0) {
        debug_printf("Code:\n");
        for (i = 0; i < sizeof(code); i++) {
            if (i % 16 == 0) {
                if (i > 0) {
                    debug_printf("\n");
                }
                debug_printf("%016lx: ", r[IP] + i);
            }

            debug_printf("%02hhx ", code[i]);
        }
        debug_printf("\n");
    } else {
        debug_printf("Unable to read code\n");
    }

    err = remote_read(remote, r[SP], stack, sizeof(stack));
    if (err == 0) {
        debug_printf("Stack:\n");
        for (i = 0; i < ARRAY_SIZE(stack); i++) {
            debug_printf(
                "%016lx: %016lx\n", r[SP] + i * sizeof(unsigned long), stack[i]
            );
        }
        debug_printf("\n");
    } else {
        debug_printf("Unable to read stack\n");
    }
}

static const struct remote_arch_ops arm_ops = {
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
    int ret = 0;
    struct remote_arm *arch;

    arch = calloc(1, sizeof(struct remote_arm));
    if (arch == NULL) {
        ret = XERR(INSUFFICIENT_MEMORY);
        goto failed;
    }

    remote->arch_ops = &arm_ops;
    remote->arch = arch;

    return 0;

failed:
    if (arch != NULL) {
        free(arch);
    }

    return ret;
}

int remote_arch_close(struct remote *remote)
{
    free(remote->arch);

    return 0;
}

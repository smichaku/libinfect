#define _GNU_SOURCE
#include <remote.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <signal.h>
#include <time.h>
#include <elf.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/wait.h>

#include <util/compiler.h>
#include <util/common.h>
#include <util/debug.h>
#include <util/file.h>
#include <util/time.h>
#include <util/xerror.h>

#include <config.h>
#include <remote-mem.h>

static int in_operable_state(struct remote *remote)
{
    if (!remote->attached) {
        return XERR(NOT_ATTACHED);
    }
    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    return 0;
}

static int read_auxv(struct remote *remote)
{
    int err;
    char path[128];
    unsigned long aux[MAX_AUXS * 2];
    size_t rd;
    unsigned int length, i;

    snprintf(path, sizeof(path), "/proc/%d/auxv", remote->pid);
    err = read_file(path, aux, sizeof(aux), &rd);
    if (err != 0) {
        return err;
    }

    length = rd / sizeof(unsigned long);
    if (length % 2 != 0) {
        return XERR(UNEXPECTED_ERROR);
    }

    for (i = 0; i < length; i += 2) {
        unsigned long id = aux[i], value = aux[i + 1];

        remote->auxv[id] = value;
    }

    return 0;
}

static int save_entry(struct remote *remote)
{
    int err;

    err = remote_read(
        remote, remote->auxv[AT_ENTRY], remote->old_entry, MAX_ENTRY_BYTES
    );
    if (err != 0) {
        return err;
    }

    return 0;
}

static int restore_entry(struct remote *remote)
{
    int err;

    err = remote_write(
        remote, remote->auxv[AT_ENTRY], remote->old_entry, MAX_ENTRY_BYTES
    );
    if (err != 0) {
        return err;
    }

    return 0;
}

/* Wait for the tracee to stop on a specific signal. */
static int wait_for_tracee(
    struct remote *remote, int stopsig, bool check_pattern
)
{
    /* 1us poll period between waits. */
    static const struct timespec PollPeriod = { 0, 1000 };

    int err;
    struct timespec due, now;

    err = clock_gettime(CLOCK_MONOTONIC, &now);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    due.tv_sec = now.tv_sec + ms2s(CONFIG_TRACEE_TIMEOUT_MS);
    due.tv_nsec = now.tv_nsec + ms2ns(CONFIG_TRACEE_TIMEOUT_MS) % s2ns(1);

    for (;;) {
        pid_t wait_err;
        int wstatus;

        wait_err = waitpid(remote->pid, &wstatus, WNOHANG);
        if (wait_err == -1) {
            return xerror(XECLibc, errno);
        }

        if (wait_err != 0) {
            /* Check if the process has terminated (gracefully or not). */
            if (WIFEXITED(wstatus) || WIFSIGNALED(wstatus)) {
                return XERR(TERMINATED);
            }

            if (WIFSTOPPED(wstatus)) {
                int sig = WSTOPSIG(wstatus);

                debug_printf(
                    "Process %d stopped with signal %d\n", remote->pid, sig
                );

                if (sig == stopsig) {
                    struct remote_regs regs;

                    if (!check_pattern) {
                        return 0;
                    }

                    /* Check the registers pattern and figure out whether the
                     * remote process was stopped because the remote function
                     * returned or due to some other reason. */
                    err = ptrace(PTRACE_GETREGS, remote->pid, NULL, &regs);
                    if (err == -1) {
                        return xerror(XECLibc, errno);
                    }

                    if (remote->arch_ops->check_pattern_regs(&regs)) {
                        return 0;
                    }
                }

                if (sig == SIGTRAP && (remote->flags & RemoteFlagSingleStep)) {
                    debug_printf("Remote debug - single step\n");
                    remote_debug_dump(remote, "SINGLE-STEP");

                    err = ptrace(PTRACE_SINGLESTEP, remote->pid, 0, 0);
                    if (err == -1) {
                        return xerror(XECLibc, errno);
                    }

                    continue;
                }

                debug_printf(
                    "Injecting unexpected signal %d to process %d\n",
                    sig,
                    remote->pid
                );

                /* Re-inject the unrelated signal. */
                err = ptrace(PTRACE_CONT, remote->pid, 0, sig);
                if (err == -1) {
                    return xerror(XECLibc, errno);
                }
            }
        }

        err = RETRY_SYSCALL(
            clock_nanosleep(CLOCK_MONOTONIC, 0, &PollPeriod, NULL)
        );
        if (err != 0) {
            return xerror(XECLibc, err);
        }

        err = clock_gettime(CLOCK_MONOTONIC, &now);
        if (err != 0) {
            return xerror(XECLibc, err);
        }

        if (timespec_cmp(&now, &due) >= 0) {
            return XERR(TIMEOUT);
        }
    }
}

int remote_open(pid_t pid, struct remote **remote, int flags)
{
    int err, ret = 0;
    struct remote *r = NULL;

    debug_printf("remote_open(pid=%d)\n", pid);

    r = calloc(1, sizeof(struct remote));
    if (r == NULL) {
        ret = XERR(INSUFFICIENT_MEMORY);
        goto failed;
    }

    r->pid = pid;
    r->flags = flags;

    err = remote_attach(r);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = read_auxv(r);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = save_entry(r);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = remote_arch_open(r);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    *remote = r;

    return 0;

failed:
    if (r != NULL) {
        if (r->attached) {
            remote_detach(r);
        }

        free(r);
    }

    return ret;
}

int remote_close(struct remote *remote)
{
    int err, ret = 0;

    debug_printf("remote_close(pid=%d)\n", remote->pid);

    if (!remote->terminated) {
        err = restore_entry(remote);
        if (err != 0) {
            ret = err;
            goto out;
        }
    }

    if (remote->attached) {
        err = remote_detach(remote);
        if (err != 0) {
            ret = err;
            goto out;
        }
    }

out:
    remote_arch_close(remote);

    free(remote);

    return ret;
}

int remote_attach(struct remote *remote)
{
    int err, ret = 0;

    debug_printf("remote_attach(pid=%d)\n", remote->pid);

    if (remote->attached) {
        return XERR(ALREADY_ATTACHED);
    }
    if (remote->terminated) {
        return XERR(TERMINATED);
    }

    err = ptrace(PTRACE_ATTACH, remote->pid, 0, 0);
    if (err == -1) {
        ret = xerror(XECLibc, errno);
        goto failed;
    }

    err = wait_for_tracee(remote, SIGSTOP, false);
    if (err != 0) {
        if (err == XERR_TERMINATED) {
            remote->terminated = 1;
        }

        ret = err;
        goto failed;
    }

    err = ptrace(PTRACE_GETREGS, remote->pid, NULL, &remote->restore.regs);
    if (err != 0) {
        ret = xerror(XECLibc, errno);
        goto failed;
    }

    remote->attached = 1;

    return 0;

failed:
    ptrace(PTRACE_DETACH, remote->pid, 0, 0);

    return ret;
}

int remote_detach(struct remote *remote)
{
    int err;

    debug_printf("remote_detach(pid=%d)\n", remote->pid);

    if (!remote->attached) {
        return XERR(NOT_ATTACHED);
    }

    if (!remote->terminated) {
        err = ptrace(PTRACE_SETREGS, remote->pid, NULL, &remote->restore.regs);
        if (err != 0) {
            return xerror(XECLibc, errno);
        }
    }

    err = ptrace(PTRACE_DETACH, remote->pid, 0, 0);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    remote->attached = 0;

    return 0;
}

int remote_read(
    struct remote *remote, uintptr_t address, void *data, size_t size
)
{
    int err;
    ssize_t rd;
    struct iovec local_iov, remote_iov;

    debug_printf(
        "remote_read(pid=%d, address=0x%" PRIxPTR ", data=%p, size=%zu)\n",
        remote->pid,
        address,
        data,
        size
    );

    err = in_operable_state(remote);
    if (err != 0) {
        return err;
    }

    local_iov.iov_base = data;
    local_iov.iov_len = size;
    remote_iov.iov_base = (void *) address;
    remote_iov.iov_len = size;
    rd = process_vm_readv(remote->pid, &local_iov, 1, &remote_iov, 1, 0);
    if (rd == -1) {
        if (errno == ENOSYS || errno == EFAULT) {
            debug_printf("Trying slow ptrace word-by-word read\n");
            return remote_ptrace_read(remote, address, data, size);
        }

        return xerror(XECLibc, errno);
    }

    return 0;
}

int remote_write(
    struct remote *remote, uintptr_t address, const void *data, size_t size
)
{
    int err;
    ssize_t wt;
    struct iovec local_iov, remote_iov;

    debug_printf(
        "remote_write(pid=%d, address=0x%" PRIxPTR ", data=%p, size=%zu)\n",
        remote->pid,
        address,
        data,
        size
    );

    err = in_operable_state(remote);
    if (err != 0) {
        return err;
    }

    local_iov.iov_base = REMOVE_CONST(void *, data);
    local_iov.iov_len = size;
    remote_iov.iov_base = (void *) address;
    remote_iov.iov_len = size;
    wt = process_vm_writev(remote->pid, &local_iov, 1, &remote_iov, 1, 0);
    if (wt == -1) {
        if (errno == ENOSYS || errno == EFAULT) {
            debug_printf("Trying slow ptrace word-by-word write\n");
            return remote_ptrace_write(remote, address, data, size);
        }

        return xerror(XECLibc, errno);
    }

    return 0;
}

int remote_call(
    struct remote *remote,
    uintptr_t function,
    const struct remote_arg *args,
    unsigned int nargs,
    struct remote_arg *retval
)
{
    int err;
    struct remote_regs regs;
    enum __ptrace_request request;

    err = in_operable_state(remote);
    if (err != 0) {
        return err;
    }

    debug_printf(
        "remote_call(pid=%d, function=0x%" PRIxPTR ")\n", remote->pid, function
    );

    memcpy(&regs, &remote->restore.regs, sizeof(struct remote_regs));

    /* Set a pattern in registers which are not required by the remote function.
     * The pattern will be used later to verify the function call completed
     * successfully. */
    remote->arch_ops->set_pattern_regs(&regs);

    err = remote->arch_ops->call(remote, &regs, function, args, nargs);
    if (err != 0) {
        return err;
    }

    err = ptrace(PTRACE_SETREGS, remote->pid, NULL, &regs);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    remote_debug_dump(remote, "PRE-CALL");

    request = (remote->flags & RemoteFlagSingleStep) ? PTRACE_SINGLESTEP :
                                                       PTRACE_CONT;
    err = ptrace(request, remote->pid, 0, 0);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    /* Wait for the tracee to finish executing the function. We expect a SIGSEGV
     * with the correct registers pattern. */
    err = wait_for_tracee(remote, SIGSEGV, true);
    if (err != 0) {
        if (err == XERR_TERMINATED) {
            remote->terminated = 1;
        }

        return err;
    }

    err = ptrace(PTRACE_GETREGS, remote->pid, NULL, &regs);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    remote_debug_dump(remote, "POST-CALL");

    err = remote->arch_ops->ret(remote, &regs, retval);
    if (err != 0) {
        return err;
    }

    /* Set fault bit so next call or syscall will account for the instruction
     * execution retry. */
    remote->fault = 1;

    return 0;
}

int remote_syscall(
    struct remote *remote,
    long number,
    const struct remote_arg *args,
    size_t nargs,
    struct remote_arg *retval
)
{
    static const int SyscallTrapSignal = SIGTRAP | 0x80;

    int err;
    struct remote_regs regs;

    err = in_operable_state(remote);
    if (err != 0) {
        return err;
    }

    debug_printf("remote_syscall(pid=%d, number=%ld)\n", remote->pid, number);

    memcpy(&regs, &remote->restore.regs, sizeof(struct remote_regs));

    err = remote->arch_ops->syscall(remote, &regs, number, args, nargs);
    if (err != 0) {
        return err;
    }

    err = ptrace(PTRACE_SETREGS, remote->pid, NULL, &regs);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    err = ptrace(PTRACE_SETOPTIONS, remote->pid, 0, PTRACE_O_TRACESYSGOOD);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    remote_debug_dump(remote, "PRE-SYSCALL");

    err = ptrace(PTRACE_SYSCALL, remote->pid, 0, 0);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    err = wait_for_tracee(remote, SyscallTrapSignal, false);
    if (err != 0) {
        if (err == XERR_TERMINATED) {
            remote->terminated = 1;
        }

        return err;
    }

    err = ptrace(PTRACE_SYSCALL, remote->pid, 0, 0);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    err = wait_for_tracee(remote, SyscallTrapSignal, false);
    if (err != 0) {
        if (err == XERR_TERMINATED) {
            remote->terminated = 1;
        }

        return err;
    }

    err = ptrace(PTRACE_GETREGS, remote->pid, NULL, &regs);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    remote_debug_dump(remote, "POST-SYSCALL");

    err = remote->arch_ops->sysret(remote, &regs, retval);
    if (err != 0) {
        return err;
    }

    remote->fault = 0;

    return 0;
}

void remote_debug_dump(
    maybe_unused struct remote *remote, maybe_unused const char *msg
)
{
#ifndef NDEBUG
    int err;
    struct remote_regs regs;

    if (getenv("REMOTE_DEBUG_DUMP") == NULL) {
        return;
    }

    debug_printf("Debug dump for remote process %d [%s]:\n", remote->pid, msg);

    if (remote->terminated) {
        debug_printf("\tProcess has terminated\n");
        return;
    }

    err = ptrace(PTRACE_GETREGS, remote->pid, NULL, &regs);
    if (err == -1) {
        debug_printf("Unable to read remote process registers\n");
        return;
    }

    remote->arch_ops->debug_dump(remote, &regs);
#endif
}

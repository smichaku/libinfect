#define _GNU_SOURCE
#include <freestanding/thread.h>

#include <sched.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/user.h>

#include <freestanding/common.h>
#include <freestanding/exit.h>
#include <freestanding/mman.h>
#include <freestanding/atomic.h>
#include <freestanding/futex.h>
#include <syscall.h>

enum
{
    ThreadStateExited = 0,
    ThreadStateExiting,
    ThreadStateJoinable,
    ThreadStateDetached,
    ThreadStateDynamic,
};

struct thread {
    void *map;
    size_t map_size;

    void *stack;

    void *(*entry)(void *);
    void *arg;
    void *retval;

    pid_t tid;

    volatile uint32_t state;
};

extern int sys_clone(
    int (*func)(void *), void *stack, int flags, void *arg, ...
);

static _Noreturn void exit_thread(struct thread *thread, void *retval)
{
    thread->retval = retval;

    fs_atomic32_cmpxchg(
        &thread->state, ThreadStateJoinable, ThreadStateExiting
    );

    thread->tid = 0;

    fs_exit(0);
}

static int start_thread(void *arg)
{
    struct thread *thread = arg;

    exit_thread(thread, thread->entry(thread->arg));

    return 0;
}

pid_t fs_gettid(void)
{
    return __syscall(SYS_gettid);
}

int fs_thread_create(
    void *(*entry)(void *), void *arg, size_t stack_size, thread_t *thread
)
{
    int err, ret = 0;
    size_t pgsize;
    size_t map_size;
    void *map = NULL;
    struct thread *sthread;
    void *stack;
    int clone_flags;
    long r;

    err = fs_pagesize(&pgsize);
    if (err != 0) {
        ret = err;
        goto out;
    }

    map_size = ALIGN_UP(sizeof(struct thread) + stack_size, (size_t) pgsize);

    err = fs_mmap(
        NULL,
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

    sthread = map;
    stack = (uint8_t *) map + sizeof(struct thread) + stack_size;

    sthread->map = map;
    sthread->map_size = map_size;
    sthread->stack = stack;
    sthread->entry = entry;
    sthread->arg = arg;
    sthread->state = ThreadStateJoinable;

    clone_flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND |
                  CLONE_THREAD | CLONE_SYSVSEM | CLONE_PARENT_SETTID |
                  CLONE_CHILD_CLEARTID | CLONE_DETACHED;

    r = sys_clone(
        start_thread,
        stack,
        clone_flags,
        sthread,
        &sthread->tid,
        NULL,
        &sthread->state
    );
    err = __syscall_err(r);
    if (err != 0) {
        ret = err;
        goto out;
    }

    *thread = sthread;

out:
    if (ret != 0) {
        if (map != NULL) {
            fs_munmap(map, stack_size);
        }
    }

    return ret;
}

int fs_thread_join(thread_t thread, void **retval)
{
    int err, ret = 0;
    uint32_t state;

    for (;;) {
        fs_atomic32_load(&thread->state, &state);

        if (state == ThreadStateExited) {
            break;
        }

        err = fs_futex_wait(&thread->state, state);
        if (err != 0 && err != -EAGAIN) {
            ret = err;
            goto out;
        }
    }

    if (retval != NULL) {
        *retval = thread->retval;
    }

out:
    fs_munmap(thread->map, thread->map_size);

    return ret;
}

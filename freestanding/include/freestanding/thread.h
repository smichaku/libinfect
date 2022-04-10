#pragma once

#include <stddef.h>
#include <sys/types.h>

typedef struct thread *thread_t;

pid_t fs_gettid(void);

int fs_thread_create(
    void *(*entry)(void *), void *arg, size_t stack_size, thread_t *thread
);

int fs_thread_join(thread_t thread, void **retval);

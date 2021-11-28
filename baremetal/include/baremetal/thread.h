#pragma once

#include <stddef.h>
#include <sys/types.h>

typedef struct thread *thread_t;

pid_t bm_gettid(void);

int bm_thread_create(
    void *(*entry)(void *), void *arg, size_t stack_size, thread_t *thread
);

int bm_thread_join(thread_t thread, void **retval);

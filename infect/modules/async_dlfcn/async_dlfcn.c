#include <stddef.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/user.h>

#include <baremetal/common.h>
#include <baremetal/string.h>
#include <baremetal/alloc.h>
#include <baremetal/thread.h>

#include <module-promise.h>

#define DEFAULT_STACK_SIZE MB(1)
#define DLERROR_MAXSIZE    128

typedef void *(*dlopen_t)(const char *, int);
typedef int (*dlclose_t)(void *);
typedef char *(*dlerror_t)(void);

typedef char dlerror_msg_t[128];

struct {
    dlopen_t dlopen;
    dlclose_t dlclose;
    dlerror_t dlerror;
} dlfcn;

struct init_args {
    dlopen_t dlopen;
    dlclose_t dlclose;
    dlerror_t dlerror;
};

struct dlopen_args {
    const char *filename;
    int flags;

    struct module_promise *promise;
};

struct dlclose_args {
    void *handle;

    struct module_promise *promise;
};

static void *dlopen_routine(void *arg)
{
    struct dlopen_args *args = arg;
    const char *filename = args->filename;
    int flags = args->flags;
    struct module_promise *promise = args->promise;
    void *handle;

    bm_free(args);

    handle = dlfcn.dlopen(filename, flags);

    promise->value = handle;
    module_promise_fulfill(promise);

    return NULL;
}

static void *dlclose_routine(void *arg)
{
    struct dlclose_args *args = arg;
    void *handle = args->handle;
    struct module_promise *promise = args->promise;
    int rv;

    bm_free(args);

    rv = dlfcn.dlclose(handle);

    promise->value = (void *) (uintptr_t) rv;
    module_promise_fulfill(promise);

    return NULL;
}

static char *default_dlerror(void)
{
    return "dlerror() is not available";
}

void async_dlfcn_init(dlopen_t dlopen, dlclose_t dlclose, dlerror_t dlerror)
{
    dlfcn.dlopen = dlopen;
    dlfcn.dlclose = dlclose;
    dlfcn.dlerror = dlerror;

    if (dlfcn.dlerror == NULL) {
        dlfcn.dlerror = default_dlerror;
    }
}

int async_dlopen(
    const char *filename, int flags, struct module_promise *promise
)
{
    int err, ret = 0;
    struct dlopen_args *args = NULL;
    thread_t thread = NULL;

    err = bm_malloc(sizeof(*args), (void **) &args);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    args->filename = filename;
    args->flags = flags;
    args->promise = promise;

    err = bm_thread_create(dlopen_routine, args, DEFAULT_STACK_SIZE, &thread);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    promise->thread = thread;

    return 0;

failed:
    if (args != NULL) {
        bm_free(args);
    }

    return ret;
}

int async_dlclose(void *handle, struct module_promise *promise)
{
    int err, ret = 0;
    struct dlclose_args *args = NULL;
    thread_t thread = NULL;

    err = bm_malloc(sizeof(*args), (void **) &args);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    args->handle = handle;
    args->promise = promise;

    err = bm_thread_create(dlclose_routine, args, DEFAULT_STACK_SIZE, &thread);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    promise->thread = thread;

    return 0;

failed:
    if (args != NULL) {
        bm_free(args);
    }

    return ret;
}

bool async_dlfcn_ready(struct module_promise *promise)
{
    if (module_promise_check(promise)) {
        bm_thread_join(promise->thread, NULL);
        return true;
    }

    return false;
}

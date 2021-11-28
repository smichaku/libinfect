#include <stddef.h>

#include "module-promise.h"

void module_promise_fulfill(struct module_promise *promise)
{
    __atomic_store_n(&promise->ready, true, __ATOMIC_RELEASE);
}

bool module_promise_check(struct module_promise *promise)
{
    return __atomic_load_n(&promise->ready, __ATOMIC_ACQUIRE);
}

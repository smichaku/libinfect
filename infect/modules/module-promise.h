#pragma once

#include <stdint.h>
#include <stdbool.h>

struct module_promise {
    void *thread;

    volatile bool ready;

    void *value;
};

void module_promise_fulfill(struct module_promise *promise);

bool module_promise_check(struct module_promise *promise);

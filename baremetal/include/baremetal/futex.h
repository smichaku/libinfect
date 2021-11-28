#pragma once

#include <stdint.h>

int bm_futex_wait(volatile uint32_t *var, uint32_t expected);

int bm_futex_wake(volatile uint32_t *var, uint32_t n);

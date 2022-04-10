#pragma once

#include <stdint.h>

int fs_futex_wait(volatile uint32_t *var, uint32_t expected);

int fs_futex_wake(volatile uint32_t *var, uint32_t n);

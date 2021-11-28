#pragma once

#include <stdint.h>

uint32_t bm_atomic32_cmpxchg(
    volatile uint32_t *var, uint32_t expected, uint32_t desired
);

uint32_t bm_atomic32_dec(volatile uint32_t *var);

void bm_atomic32_load(volatile uint32_t *var, uint32_t *val);

void bm_atomic32_store(volatile uint32_t *var, uint32_t val);

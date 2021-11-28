#pragma once

#include <stddef.h>

int bm_malloc(size_t size, void **ptr);

int bm_free(void *ptr);

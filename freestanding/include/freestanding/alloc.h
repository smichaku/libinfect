#pragma once

#include <stddef.h>

int fs_malloc(size_t size, void **ptr);

int fs_free(void *ptr);

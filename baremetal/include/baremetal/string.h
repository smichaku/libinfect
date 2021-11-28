#pragma once

#include <stddef.h>

void *bm_memset(void *dest, int c, size_t n);

char *bm_stpncpy(char *d, const char *s, size_t n);

char *bm_strncpy(char *dest, const char *src, size_t n);

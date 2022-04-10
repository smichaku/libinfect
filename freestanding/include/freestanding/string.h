#pragma once

#include <stddef.h>

void *fs_memset(void *dest, int c, size_t n);

char *fs_stpncpy(char *d, const char *s, size_t n);

char *fs_strncpy(char *dest, const char *src, size_t n);

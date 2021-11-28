#pragma once

#include <stdint.h>

#include <remote.h>

int remote_malloc(struct remote *remote, size_t size, uintptr_t *ptr);

int remote_free(struct remote *remote, uintptr_t ptr);

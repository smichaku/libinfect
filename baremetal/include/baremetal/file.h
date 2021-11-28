#pragma once

#include <sys/types.h>

int bm_open(const char *filename, int flags, mode_t mode, int *fd);

int bm_close(int fd);

int bm_dup(int oldfd, int *newfd);

int bm_dup2(int oldfd, int newfd);

int bm_read(int fd, void *buf, size_t count, size_t *rd);

int bm_write(int fd, const void *buf, size_t count, size_t *wt);

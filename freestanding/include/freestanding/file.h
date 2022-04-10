#pragma once

#include <sys/types.h>

int fs_open(const char *filename, int flags, mode_t mode, int *fd);

int fs_close(int fd);

int fs_dup(int oldfd, int *newfd);

int fs_dup2(int oldfd, int newfd);

int fs_read(int fd, void *buf, size_t count, size_t *rd);

int fs_write(int fd, const void *buf, size_t count, size_t *wt);

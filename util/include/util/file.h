#pragma once

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Get the size of a file given its path. */
int filesize(const char *filename, size_t *size);

/* Like read(2) but persistently attempts to read all data from the buffer. */
int readall(int fd, void *buf, size_t size, size_t *read_bytes);

/* Like write(2) but persistently attempts to write all data to the buffer. */
int writeall(int fd, const void *buf, size_t count);

/* Read up to 'bufsize' bytes from a file referenced by 'path' to 'buf'. The
 * number of bytes actually read is returned in 'bytes_read'. */
int read_file(const char *path, void *buf, size_t bufsize, size_t *bytes_read);

/* Write 'count' bytes from 'buf' to a file referenced by 'path'. The file is
 * created with 'mode' if it does not exist, otherwise it is truncated. */
int write_file(const char *path, mode_t mode, const void *buf, size_t count);

#ifdef __cplusplus
}
#endif

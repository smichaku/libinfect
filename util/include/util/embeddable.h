#pragma once

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Unload an embeddable to a file. If the file exist it will be overwritten.
 *
 * @name - Name of the embeddable.
 * @path - Path of the target file.
 * @mode - Mode for the target file.
 */
int embeddable_unload(const char *name, const char *path, mode_t mode);

#ifdef __cplusplus
}
#endif

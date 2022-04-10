#pragma once

#include <link.h>
#include <stdbool.h>
#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    /* Start and end addresses. */
    void *start;
    void *end;

    /* Protection i.e. PROT_* flags. */
    int prot;

    /* Sharing i.e. MAP_PRIVATE or MAP_SHARED flags. */
    int sharing;

    /* File offset. */
    unsigned long long offset;

    /* Device major:minor. */
    unsigned int dev_major;
    unsigned int dev_minor;

    /* Filesystem inode. */
    unsigned long inode;

    /* File path. */
    const char *path;
} process_map_info_t;

typedef int (*map_callback_t)(const process_map_info_t *info, void *context);

/* Get the address, size and header of the given section in the current
 * process. */
int get_section_info(
    const char *name, void **addr, size_t *size, ElfW(Shdr) *header
);

/* Iterate over the given process' maps invoking 'callback' on each.
 * The function passes the maps information in a process_map_info_t as well as
 * the given 'context' to the callback function. The callback function should
 * return 0 unless iterating the maps should stop in which case the return value
 * of iterate_maps() will be the same as that of the callback function. */
int iterate_maps(pid_t pid, map_callback_t callback, void *context);

/* Spawn a new process running the given executable. The 'argv' parameter is the
 * same as in execv(). This function will return only after the new process
 * calls execv().
 *
 * If 'daemonize' is true the new process will be detached from the calling
 * parent. Otherwise the new process will remain a child of the calling parent
 * and its ID will be available in the 'pid' output parameter. In the latter
 * case is up to the caller to wait on this child in order to reap it and
 * cleanup its OS resources. */
int spawn(char * const argv[], bool daemonize, pid_t *pid);

#ifdef __cplusplus
}
#endif

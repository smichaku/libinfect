#pragma once

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Inject a dynamic shared object into a process.
 *
 * @pid: the pid of the process.
 * @lib: the DSO's path.
 * @handle: a handle which may be used for ejecting.
 */
int inject_dso(pid_t pid, const char *lib, void **handle);

/* Eject a previously injected dynamic shared object from a process.
 *
 * @pid: the pid of the process.
 * @handle: a handle to the DSO to eject.
 */
int eject_dso(pid_t pid, void *handle);

/* Inject a dynamic shared object into a process with a risk of undefined
 * behavior.
 *
 * @pid: the pid of the process.
 * @lib: the DSO's path.
 * @handle: a handle which may be used for ejecting.
 */
int inject_dso_unsafe(pid_t pid, const char *lib, void **handle);

/* Eject a previously (unsafely-)injected dynamic shared object from a process.
 *
 * @pid: the pid of the process.
 * @handle: a handle to the DSO to eject.
 */
int eject_dso_unsafe(pid_t pid, void *handle);

/* Inject a module into a process.
 *
 * @pid: the pid of the process.
 * @lib: the module's path.
 * @handle: a handle which may be used for ejecting.
 */
int inject_module(pid_t pid, const char *path, void **handle);

/* Eject a previously injected module from a process.
 * @pid: the pid of the process.
 * @handle: a handle to the module to eject.
 */
int eject_module(pid_t pid, void *handle);

/* Inject a blob of code into a process.
 *
 * @pid: the pid of the process.
 * @code: the code buffer.
 * @code_size: the size of the code buffer.
 * @entry: offset to code entry point.
 * @handle: a handle which may be used for ejecting.
 */
int inject_code(
    pid_t pid, const void *code, size_t code_size, size_t entry, void **handle
);

/* Eject a previously injected code blob from a process.
 *
 * @pid: the pid of the process.
 * @handle: a handle to the code blob to eject.
 */
int eject_code(pid_t pid, void *handle);

#ifdef __cplusplus
}
#endif

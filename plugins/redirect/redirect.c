#include <stdio.h>
#include <fcntl.h>
#include <sys/cdefs.h>
#include <unistd.h>

#include <baremetal/debug.h>
#include <baremetal/file.h>
#include <baremetal/process.h>
#include <baremetal/format.h>

/* TODO: Use a prime slot for this. */
#define TMPDIR "/tmp"

static int g_old_stdout = -1;
static int g_old_stderr = -1;

static int redirect(int fd, const char *path, int *oldfd)
{
    int err, ret = 0;
    int pathfd = -1;

    *oldfd = -1;

    err = bm_open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666, &pathfd);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = bm_dup(fd, oldfd);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = bm_dup2(pathfd, fd);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    return 0;

failed:
    if (*oldfd != -1) {
        bm_dup2(*oldfd, fd);
    }

    if (pathfd != -1) {
        bm_close(pathfd);
    }

    return ret;
}

static void reset(void)
{
    if (g_old_stdout != -1) {
        bm_dup2(g_old_stdout, STDOUT_FILENO);
        bm_close(g_old_stdout);
        g_old_stdout = -1;
    }

    if (g_old_stderr != -1) {
        bm_dup2(g_old_stderr, STDERR_FILENO);
        bm_close(g_old_stderr);
        g_old_stderr = -1;
    }
}

static void __attribute__((constructor)) init(void)
{
    int err;
    pid_t pid;
    char path[128];

    pid = bm_getpid();

    bm_snprintf(path, sizeof(path), "%s/redirect-%d.out", TMPDIR, pid);
    err = redirect(STDOUT_FILENO, path, &g_old_stdout);
    if (err != 0) {
        reset();
        return;
    }

    bm_snprintf(path, sizeof(path), "%s/redirect-%d.err", TMPDIR, pid);
    err = redirect(STDERR_FILENO, path, &g_old_stderr);
    if (err != 0) {
        reset();
        return;
    }
}

static void __attribute__((destructor)) fini(void)
{
    reset();
}

#include <stdio.h>
#include <fcntl.h>
#include <sys/cdefs.h>
#include <unistd.h>

#include <freestanding/debug.h>
#include <freestanding/file.h>
#include <freestanding/process.h>
#include <freestanding/format.h>

/* TODO: Use a prime slot for this. */
#define TMPDIR "/tmp"

static int g_old_stdout = -1;
static int g_old_stderr = -1;

static int redirect(int fd, const char *path, int *oldfd)
{
    int err, ret = 0;
    int pathfd = -1;

    *oldfd = -1;

    err = fs_open(path, O_CREAT | O_TRUNC | O_WRONLY, 0666, &pathfd);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = fs_dup(fd, oldfd);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    err = fs_dup2(pathfd, fd);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    return 0;

failed:
    if (*oldfd != -1) {
        fs_dup2(*oldfd, fd);
    }

    if (pathfd != -1) {
        fs_close(pathfd);
    }

    return ret;
}

static void reset(void)
{
    if (g_old_stdout != -1) {
        fs_dup2(g_old_stdout, STDOUT_FILENO);
        fs_close(g_old_stdout);
        g_old_stdout = -1;
    }

    if (g_old_stderr != -1) {
        fs_dup2(g_old_stderr, STDERR_FILENO);
        fs_close(g_old_stderr);
        g_old_stderr = -1;
    }
}

static void __attribute__((constructor)) init(void)
{
    int err;
    pid_t pid;
    char path[128];

    pid = fs_getpid();

    fs_snprintf(path, sizeof(path), "%s/redirect-%d.out", TMPDIR, pid);
    err = redirect(STDOUT_FILENO, path, &g_old_stdout);
    if (err != 0) {
        reset();
        return;
    }

    fs_snprintf(path, sizeof(path), "%s/redirect-%d.err", TMPDIR, pid);
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

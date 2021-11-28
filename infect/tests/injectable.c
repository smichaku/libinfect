#include <fcntl.h>
#include <unistd.h>

static void __attribute__((constructor)) init(void)
{
    int fd = -1;

    fd = open("inject-ok", O_CREAT | O_TRUNC | O_WRONLY, 0666);
    if (fd < 0) {
        goto out;
    }

    if (write(fd, "1", 1) != 1) {
        goto out;
    }

out:
    if (fd != -1) {
        close(fd);
    }
}

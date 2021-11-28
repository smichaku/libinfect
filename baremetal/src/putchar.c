#include <unistd.h>

#include <baremetal/file.h>
#include <printf.h>

void _putchar(char character)
{
    size_t written;

    bm_write(STDOUT_FILENO, &character, 1, &written);
}

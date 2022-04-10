#include <unistd.h>

#include <freestanding/file.h>
#include <printf.h>

void _putchar(char character)
{
    size_t written;

    fs_write(STDOUT_FILENO, &character, 1, &written);
}

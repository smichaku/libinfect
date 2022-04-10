#include <freestanding/string.h>

void *fs_memset(void *dest, int c, size_t n)
{
    unsigned char *s = dest;

    for (; n > 0; n--, s++) {
        *s = (char) c;
    }

    return dest;
}

char *fs_stpncpy(char *d, const char *s, size_t n)
{
    for (; n && (*d = *s); n--, s++, d++) {
    }

    fs_memset(d, 0, n);

    return d;
}

char *fs_strncpy(char *d, const char *s, size_t n)
{
    fs_stpncpy(d, s, n);

    return d;
}

#include <baremetal/string.h>

void *bm_memset(void *dest, int c, size_t n)
{
    unsigned char *s = dest;

    for (; n > 0; n--, s++) {
        *s = (char) c;
    }

    return dest;
}

char *bm_stpncpy(char *d, const char *s, size_t n)
{
    for (; n && (*d = *s); n--, s++, d++) {
    }

    bm_memset(d, 0, n);

    return d;
}

char *bm_strncpy(char *d, const char *s, size_t n)
{
    bm_stpncpy(d, s, n);

    return d;
}

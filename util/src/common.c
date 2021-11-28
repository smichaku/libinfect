#include <util/common.h>

#include <unistd.h>
#include <sys/types.h>

size_t pagesize(void)
{
    static size_t pgsize = 0;

    if (pgsize == 0) {
        pgsize = (size_t) sysconf(_SC_PAGESIZE);
    }

    return pgsize;
}

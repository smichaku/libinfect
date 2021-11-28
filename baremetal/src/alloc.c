#include <baremetal/alloc.h>

#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <sys/mman.h>

#include <baremetal/mman.h>

/* XXX: The library currently implements malloc() and free() with simple page
 * allocation using mmap(). Proper heap should be implemented in the future. */

int bm_malloc(size_t size, void **ptr)
{
    int err;
    size_t extsize;
    void *map;

    if (size == 0 || ptr == NULL) {
        return -EINVAL;
    }

    extsize = size + sizeof(size_t);

    err = bm_mmap(
        NULL,
        extsize,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0,
        &map
    );
    if (err != 0) {
        return err;
    }

    *((size_t *) map) = extsize;

    *ptr = (uint8_t *) map + sizeof(size_t);

    return 0;
}

int bm_free(void *ptr)
{
    size_t extsize;

    if (ptr == NULL) {
        return -EINVAL;
    }

    extsize = *((size_t *) ptr);

    return bm_munmap((uint8_t *) ptr + sizeof(size_t), extsize);
}

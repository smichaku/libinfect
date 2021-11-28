#include <util/elf.h>

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <link.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>

#include <util/common.h>
#include <util/file.h>
#include <util/xerror.h>

struct elf {
    int fd;
    size_t size;
    char *mem;
};

struct search_section_info {
    const char *name;
    const ElfW(Shdr) *header;
    const void *data;
};

static bool validate_elf(void *mem)
{
    const ElfW(Ehdr) *ehdr = mem;

    return memcmp(ehdr->e_ident, ELFMAG, SELFMAG) == 0;
}

static int iterate_sections(
    struct elf *elf, elf_section_callback_t callback, void *context
)
{
    const ElfW(Ehdr) *ehdr;
    const ElfW(Shdr) *stable, *sstr;
    unsigned int i;

    ehdr = (const ElfW(Ehdr) *) elf->mem;
    stable = (const ElfW(Shdr) *) (elf->mem + ehdr->e_shoff);
    sstr = &stable[ehdr->e_shstrndx];

    for (i = 0; i < ehdr->e_shnum; i++) {
        const ElfW(Shdr) *shdr = &stable[i];
        const char *sname = elf->mem + sstr->sh_offset + shdr->sh_name;
        const void *data = elf->mem + shdr->sh_offset;
        int status;

        status = callback(sname, shdr, data, context);
        if (status != 0) {
            return status;
        }
    }

    return 0;
}

static int search_section_callback(
    const char *name,
    const ElfW(Shdr) *section_hdr,
    const void *section_data,
    void *context
)
{
    struct search_section_info *info = context;

    if (strcmp(name, info->name) != 0) {
        return 0;
    }

    info->header = section_hdr;
    info->data = section_data;

    return 1;
}

int elf_open(const char *path, elf_t *elf)
{
    int err, ret = 0;
    int fd = -1;
    void *mem = NULL;
    size_t size;
    struct elf *e = NULL;

    if (path == NULL || elf == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    fd = open(path, O_RDONLY);
    if (fd == -1) {
        ret = xerror(XECLibc, errno);
        goto failed;
    }

    err = filesize(path, &size);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    mem = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        ret = xerror(XECLibc, errno);
        goto failed;
    }

    if (!validate_elf(mem)) {
        ret = XERR(BAD_FORMAT);
        goto failed;
    }

    e = calloc(1, sizeof(struct elf));
    if (e == NULL) {
        ret = XERR(INSUFFICIENT_MEMORY);
        goto failed;
    }

    e->fd = fd;
    e->size = size;
    e->mem = mem;

    *elf = e;

    return 0;

failed:
    if (e != NULL) {
        free(e);
    }

    if (mem != NULL) {
        munmap(mem, size);
    }

    if (fd != -1) {
        close(fd);
    }

    return ret;
}

int elf_open_mem(void *mem, size_t size, elf_t *elf)
{
    int ret = 0;
    struct elf *e = NULL;

    if (!validate_elf(mem)) {
        ret = XERR(BAD_FORMAT);
        goto failed;
    }

    e = calloc(1, sizeof(struct elf));
    if (e == NULL) {
        ret = XERR(INSUFFICIENT_MEMORY);
        goto failed;
    }

    e->fd = -1;
    e->size = size;
    e->mem = mem;

    *elf = e;

    return 0;

failed:
    if (e != NULL) {
        free(e);
    }

    return ret;
}

void elf_close(elf_t elf)
{
    if (elf == NULL) {
        return;
    }

    if (elf->fd != -1) {
        munmap(elf->mem, elf->size);
        close(elf->fd);
    }

    free(elf);
}

int elf_header(elf_t elf, const ElfW(Ehdr) **header)
{
    if (elf == NULL || header == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    *header = (const ElfW(Ehdr) *) elf->mem;

    return 0;
}

int elf_section(
    elf_t elf, const char *name, const ElfW(Shdr) **header, const void **data
)
{
    int status;
    struct search_section_info info;

    if (elf == NULL || name == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    info.name = name;

    status = iterate_sections(elf, search_section_callback, &info);
    if (status == 0) {
        return XERR(NOT_FOUND);
    }

    *header = info.header;
    if (data != NULL) {
        *data = info.data;
    }

    return 0;
}

int elf_iterate_sections(
    elf_t elf, elf_section_callback_t callback, void *context
)
{
    if (elf == NULL || callback == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    return iterate_sections(elf, callback, context);
}

int elf_symbol(elf_t elf, const char *symbol, ElfW(Addr) *addr)
{
    int err;
    const ElfW(Shdr) *dynsym_hdr, *dynstr_hdr;
    const ElfW(Sym) *sym;
    const void *dynsym, *dynstr;
    ElfW(Word) dynsym_entsize;
    unsigned int i;

    if (elf == NULL || symbol == NULL || addr == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = elf_section(elf, ".dynsym", &dynsym_hdr, &dynsym);
    if (err != 0) {
        return err;
    }

    err = elf_section(elf, ".dynstr", &dynstr_hdr, &dynstr);
    if (err != 0) {
        return err;
    }

    dynsym_entsize = dynsym_hdr->sh_entsize;
    if (dynsym_entsize == 0) {
        dynsym_entsize = sizeof(ElfW(Sym));
    }

    for (i = 0; i < dynsym_hdr->sh_size / dynsym_entsize; i++) {
        const char *name;

        sym = (const ElfW(Sym) *) dynsym + i;
        name = (const char *) dynstr + sym->st_name;

        if (strcmp(symbol, name) == 0) {
            *addr = sym->st_value;
            return 0;
        }
    }

    return XERR(NOT_FOUND);
}

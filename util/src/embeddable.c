#include <util/embeddable.h>

#include <stddef.h>
#include <stdio.h>
#include <limits.h>

#include <util/file.h>
#include <util/elf.h>

int embeddable_unload(const char *name, const char *path, mode_t mode)
{
    int err, ret = 0;
    elf_t elf = NULL;
    const ElfW(Shdr) *header = NULL;
    const void *data = NULL;
    char section_name[PATH_MAX];

    err = elf_open("/proc/self/exe", &elf);
    if (err != 0) {
        ret = err;
        goto out;
    }

    snprintf(section_name, sizeof(section_name) - 1, ".embed:%s", name);
    section_name[sizeof(section_name) - 1] = '\0';

    err = elf_section(elf, section_name, &header, &data);
    if (err != 0) {
        ret = err;
        goto out;
    }


    err = write_file(path, mode, data, header->sh_size);
    if (err != 0) {
        ret = err;
        goto out;
    }

out:
    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

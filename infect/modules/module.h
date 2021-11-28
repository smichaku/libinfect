#pragma once

#include <stddef.h>
#include <stdint.h>
#include <link.h>

#define MODULE_MAX_RELOC_SECTIONS 8

#define MODULE_MAGIC 0xAE0247FE

struct module_reloc_section {
    /* Offset to the module's relocation section (.rel or .rel.dyn) from the
     * 'module' field. */
    size_t addr;

    /* Relocation section type (either SHT_REL or SHT_RELA). */
    ElfW(Word) type;

    /* Number of entries in the relocation section. */
    unsigned int length;
};

struct module_header {
    /* The total size of the module layout including the header. */
    size_t layout_size;

    /* Module magic. */
    unsigned int magic;

    /* Number of relocation sections. */
    unsigned int nrelocs;

    /* Relocations sections info. */
    struct module_reloc_section relocs[MODULE_MAX_RELOC_SECTIONS];

    /* Offset to the module's dynamic symbol section (.dynsym) from the 'module'
     * field. */
    size_t dynsym_addr;

    /* Number of entries in the dynamic symbol section. */
    unsigned int dynsym_length;

    /* Offsets to the module's .init_array and .fini_array sections from the
     * 'module' field. */
    size_t init_array_addr;
    size_t fini_array_addr;

    /* Number of entries in .init_array and .fini_array. */
    unsigned int init_array_length;
    unsigned int fini_array_length;

    /* Offset to the module's entry and exit functions which are the first
     * entries in the .init_array and .fini_array respectively. */
    size_t entry;
    size_t exit;

    /* The module's content. */
    uint8_t module[1];
};

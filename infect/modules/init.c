#include <stdint.h>
#include <link.h>
#include <elf.h>

#include "module.h"
#include "resolve.h"

#if UINTPTR_MAX > 0xffffffff
#define R_TYPE ELF64_R_TYPE
#define R_SYM  ELF64_R_SYM
#else
#define R_TYPE ELF32_R_TYPE
#define R_SYM  ELF32_R_SYM
#endif

typedef void (*init_func_t)(void);
typedef void (*fini_func_t)(void);

static ElfW(Addr) resolve_symbol(
    const struct module_header *header, ElfW(Word) symidx
)
{
    uintptr_t base;
    const ElfW(Sym) *dynsym;

    if (symidx >= header->dynsym_length) {
        return 0;
    }

    base = (uintptr_t) header->module;
    dynsym = (const ElfW(Sym) *) (base + header->dynsym_addr);
    return dynsym[symidx].st_value;
}

static void parse_reloc(
    const void *reloc_data,
    unsigned int idx,
    ElfW(Word) type,
    ElfW(Addr) *offset,
    ElfW(Xword) *info
)
{
    switch (type) {
    case SHT_RELA: {
        const ElfW(Rela) *reloc = (const ElfW(Rela) *) reloc_data;

        *offset = reloc[idx].r_offset;
        *info = reloc[idx].r_info;
        break;
    }
    case SHT_REL: {
        const ElfW(Rel) *reloc = (const ElfW(Rel) *) reloc_data;

        *offset = reloc[idx].r_offset;
        *info = reloc[idx].r_info;
        break;
    }
    }
}

static void fix_reloc(
    const struct module_header *header, ElfW(Addr) offset, ElfW(Xword) info
)
{
    uintptr_t base = (uintptr_t) header->module;
    ElfW(Addr) *addr = (ElfW(Addr) *) (base + offset);

    switch (R_TYPE(info)) {
#if defined(__x86_64__)
    case R_X86_64_GLOB_DAT:
    case R_X86_64_JUMP_SLOT:
#elif defined(__arm__)
    case R_ARM_GLOB_DAT:
    case R_ARM_JUMP_SLOT:
#endif
        *addr = base + resolve_symbol(header, R_SYM(info));
        break;
#if defined(__x86_64__)
    case R_X86_64_RELATIVE:
#elif defined(__arm__)
    case R_ARM_RELATIVE:
#endif
        *addr += base;
        break;
    }
}

static void relocate(
    const struct module_header *header, const struct module_reloc_section *reloc
)
{
    uintptr_t base = (uintptr_t) header->module;
    const void *reloc_array = (void *) (base + reloc->addr);
    unsigned int i;

    for (i = 0; i < reloc->length; i++) {
        ElfW(Addr) offset = 0;
        ElfW(Xword) info = 0;

        parse_reloc(reloc_array, i, reloc->type, &offset, &info);
        fix_reloc(header, offset, info);
    }
}

static void __attribute__((constructor(0), used)) module_init(void *layout)
{
    const struct module_header *header;
    uintptr_t base;
    const ElfW(Addr) *init_array;
    unsigned int i;

    header = layout;
    base = (uintptr_t) header->module;

    for (i = 0; i < header->nrelocs; i++) {
        relocate(header, &header->relocs[i]);
    }

    /* First entry in the array is this function, so we skip it. */
    init_array = (const ElfW(Addr) *) (base + header->init_array_addr);
    for (i = 1; i < header->init_array_length; i++) {
        init_func_t f = (init_func_t) init_array[i];

        if (f != NULL) {
            f();
        }
    }
}

static void __attribute__((destructor(0), used)) module_fini(void *layout)
{
    const struct module_header *header;
    uintptr_t base;
    const ElfW(Addr) *fini_array;
    unsigned int i;

    header = layout;
    base = (uintptr_t) header->module;
    fini_array = (const ElfW(Addr) *) (base + header->fini_array_addr);

    /* First entry in the array is this function, so we skip it. */
    for (i = 1; i < header->fini_array_length; i++) {
        fini_func_t f = (fini_func_t) fini_array[i];

        if (f != NULL) {
            f();
        }
    }
}

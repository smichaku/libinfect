#include <remote-module.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>
#include <elf.h>
#include <link.h>

#include <util/compiler.h>
#include <util/common.h>
#include <util/debug.h>
#include <util/xerror.h>
#include <util/elf.h>
#include <remote.h>
#include <remote-mem.h>
#include <remote-handle.h>
#include <module.h>

#if defined(__64BIT)
#define R_TYPE ELF64_R_TYPE
#elif defined(__32BIT)
#define R_TYPE ELF32_R_TYPE
#endif

struct max_vaddr_info {
    size_t max_vaddr;
};

struct reloc_info {
    /* Number of RELA and REL sections found. */
    unsigned int found;

    /* Info on relocation sections. */
    struct module_reloc_section relocs[MODULE_MAX_RELOC_SECTIONS];
};

struct section_write_info {
    struct remote *remote;
    uintptr_t remote_module;
};

static int max_vaddr_callback(
    unused const char *name,
    const ElfW(Shdr) *section_hdr,
    unused const void *section_data,
    void *arg
)
{
    struct max_vaddr_info *info = arg;
    ElfW(Addr) end = section_hdr->sh_addr + section_hdr->sh_size;

    if (end > info->max_vaddr) {
        info->max_vaddr = end;
    }

    return 0;
}

static int get_module_size(elf_t elf, size_t *size)
{
    int err;
    struct max_vaddr_info info = { 0 };

    err = elf_iterate_sections(elf, max_vaddr_callback, &info);
    if (err != 0) {
        return err;
    }

    *size = info.max_vaddr;

    return 0;
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

static int reloc_info_callback(
    const char *name,
    const ElfW(Shdr) *section_hdr,
    const void *section_data,
    void *arg
)
{
    struct reloc_info *reloc_info = arg;
    ElfW(Word) type = section_hdr->sh_type;
    unsigned int i;

    UNUSED(name);

    if (type == SHT_RELA || type == SHT_REL) {
        struct module_reloc_section *module_reloc_section;

        if (reloc_info->found == MODULE_MAX_RELOC_SECTIONS) {
            return XERR(BAD_FORMAT);
        }

        module_reloc_section = &reloc_info->relocs[reloc_info->found++];
        module_reloc_section->addr = section_hdr->sh_addr;
        module_reloc_section->type = type;
        module_reloc_section->length = section_hdr->sh_size /
                                       section_hdr->sh_entsize;

        /* Only RELATIVE relocations are supported. */
        for (i = 0; i < module_reloc_section->length; i++) {
            ElfW(Addr) offset;
            ElfW(Xword) info;

            parse_reloc(section_data, i, section_hdr->sh_type, &offset, &info);

            switch (R_TYPE(info)) {
#if defined(__x86_64__)
            case R_X86_64_GLOB_DAT:
            case R_X86_64_JUMP_SLOT:
            case R_X86_64_RELATIVE:
                break;
#elif defined(__arm__) || defined(__thumb__)
            case R_ARM_GLOB_DAT:
            case R_ARM_JUMP_SLOT:
            case R_ARM_RELATIVE:
                break;
#endif
            default:
                return XERR(BAD_FORMAT);
            }
        }
    }

    return 0;
}

static int get_module_info(elf_t elf, struct module_header *header)
{
    int err;
    size_t module_size;
    const void *section;
    const ElfW(Shdr) *dyn_hdr, *dynsym_hdr, *init_array_hdr, *fini_array_hdr;
    const ElfW(Dyn) *dyn, *dyni;
    struct reloc_info reloc_info;
    unsigned int dynsym_length;
    const ElfW(Word) *init_array, *fini_array;
    unsigned int init_length, fini_length;
    unsigned int i;

    err = get_module_size(elf, &module_size);
    if (err != 0) {
        return err;
    }

    err = elf_section(elf, ".dynamic", &dyn_hdr, &section);
    if (err != 0) {
        return err;
    }
    dyn = section;

    /* Check that the module DSO is a standalone library. */
    for (dyni = dyn; dyni->d_tag != DT_NULL; dyni++) {
        if (dyni->d_tag == DT_NEEDED) {
            return XERR(BAD_FORMAT);
        }
    }

    memset(&reloc_info, 0, sizeof(reloc_info));
    err = elf_iterate_sections(elf, reloc_info_callback, &reloc_info);
    if (err != 0) {
        return err;
    }

    err = elf_section(elf, ".dynsym", &dynsym_hdr, &section);
    if (err != 0) {
        return err;
    }
    dynsym_length = dynsym_hdr->sh_size / sizeof(ElfW(Sym));

    err = elf_section(elf, ".init_array", &init_array_hdr, &section);
    if (err != 0) {
        return err;
    }
    init_array = section;
    init_length = init_array_hdr->sh_size / sizeof(ElfW(Addr));

    err = elf_section(elf, ".fini_array", &fini_array_hdr, &section);
    if (err != 0) {
        return err;
    }
    fini_array = section;
    fini_length = fini_array_hdr->sh_size / sizeof(ElfW(Addr));

    if (init_length < 1 || fini_length < 1) {
        /* A module must have at least one init/fini functions (i.e.
         * module_init() and module_fini()). */
        return XERR(BAD_FORMAT);
    }

    header->layout_size = sizeof(struct module_header) + module_size;

    for (i = 0; i < reloc_info.found; i++) {
        header->relocs[i] = reloc_info.relocs[i];
    }
    header->nrelocs = reloc_info.found;

    header->dynsym_addr = dynsym_hdr->sh_addr;
    header->dynsym_length = dynsym_length;

    header->init_array_addr = init_array_hdr->sh_addr;
    header->init_array_length = init_length;

    header->fini_array_addr = fini_array_hdr->sh_addr;
    header->fini_array_length = fini_length;

    header->entry = *init_array;
    header->exit = *fini_array;

    return 0;
}

static int section_write_callback(
    const char *name,
    const ElfW(Shdr) *section_hdr,
    const void *section_data,
    void *arg
)
{
    int err;
    struct section_write_info *swi = arg;
    ElfW(Word) type = section_hdr->sh_type;
    ElfW(Word) flags = section_hdr->sh_flags;
    ElfW(Xword) size = section_hdr->sh_size;

    UNUSED(name);

    if (type != SHT_NOBITS && (flags & SHF_ALLOC) && size > 0) {
        uintptr_t remote_address = swi->remote_module + section_hdr->sh_addr;

        err = remote_write(swi->remote, remote_address, section_data, size);
        if (err != 0) {
            return err;
        }
    }

    return 0;
}

static int do_remote_inject(
    struct remote *remote, uintptr_t remote_module, elf_t elf
)
{
    int err;
    struct section_write_info swi;

    swi.remote = remote;
    swi.remote_module = remote_module;

    err = elf_iterate_sections(elf, section_write_callback, &swi);
    if (err != 0) {
        return err;
    }

    return 0;
}

int remote_load_module(struct remote *remote, elf_t elf, void **handle)
{
    int err, ret = 0;
    struct module_header module_header;
    size_t map_size = 0;
    uintptr_t remote_layout = 0, remote_module, remote_entry;
    struct remote_arg args[1];
    void *module_handle;

    module_header.magic = MODULE_MAGIC;

    err = get_module_info(elf, &module_header);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    map_size = PAGE_ALIGN_UP(module_header.layout_size);
    err = remote_mmap(
        remote,
        0,
        map_size,
        PROT_READ | PROT_WRITE | PROT_EXEC,
        MAP_PRIVATE | MAP_ANONYMOUS,
        -1,
        0,
        &remote_layout
    );
    if (err != 0) {
        ret = err;
        goto failed;
    }

    module_handle = encode_remote_handle(remote_layout, map_size);

    err = remote_write(
        remote, remote_layout, &module_header, sizeof(struct module_header)
    );
    if (err != 0) {
        ret = err;
        goto failed;
    }

    remote_module = remote_layout + offsetof(struct module_header, module);
    err = do_remote_inject(remote, remote_module, elf);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    remote_entry = remote_module + module_header.entry;

    debug_printf(
        "MODULE: layout=0x%" PRIxPTR ", module=0x%" PRIxPTR
        ", entry=0x%" PRIxPTR ", handle=%p\n",
        remote_layout,
        remote_module,
        remote_entry,
        module_handle
    );

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = remote_layout;
    err = remote_call(remote, remote_entry, args, ARRAY_SIZE(args), NULL);
    if (err != 0) {
        ret = err;
        goto failed;
    }

    *handle = module_handle;

    return 0;

failed:
    if (remote_layout != 0) {
        remote_munmap(remote, remote_layout, map_size);
    }

    return ret;
}

int remote_unload_module(struct remote *remote, void *handle)
{
    int err;
    uintptr_t remote_layout, remote_module, remote_exit;
    size_t map_size;
    struct module_header module_header;
    struct remote_arg args[1];

    decode_remote_handle(handle, &remote_layout, &map_size);

    err = remote_read(
        remote, remote_layout, &module_header, sizeof(module_header)
    );
    if (err != 0) {
        return err;
    }

    if (module_header.magic != MODULE_MAGIC) {
        return XERR(BAD_FORMAT);
    }

    remote_module = remote_layout + offsetof(struct module_header, module);
    remote_exit = remote_module + module_header.exit;

    debug_printf(
        "MODULE: layout=0x%" PRIxPTR ", module=0x%" PRIxPTR ", exit=0x%" PRIxPTR
        ", handle=%p\n",
        remote_layout,
        remote_module,
        remote_exit,
        handle
    );

    args[0].type = RemoteArgPointer;
    args[0].arg.v_pointer = remote_layout;
    err = remote_call(remote, remote_exit, args, ARRAY_SIZE(args), NULL);
    if (err != 0) {
        return err;
    }

    err = remote_munmap(remote, remote_layout, map_size);
    if (err != 0) {
        return err;
    }

    return 0;
}

uintptr_t remote_module_base(unused struct remote *remote, void *handle)
{
    uintptr_t remote_layout;
    size_t map_size;

    decode_remote_handle(handle, &remote_layout, &map_size);

    return remote_layout + offsetof(struct module_header, module);
}

int remote_module_resolve(
    struct remote *remote,
    void *handle,
    elf_t elf,
    const char *symbol,
    uintptr_t *addr
)
{
    int err;
    ElfW(Addr) elf_addr;

    err = elf_symbol(elf, symbol, &elf_addr);
    if (err != 0) {
        return err;
    }

    *addr = remote_module_base(remote, handle) + elf_addr;

    debug_printf(
        "remote_module_resolve(handle=%p, symbol=%s) -> 0x%" PRIxPTR "\n",
        handle,
        symbol,
        *addr
    );

    return 0;
}

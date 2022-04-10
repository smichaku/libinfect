#pragma once

#include <link.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct elf *elf_t;

typedef int (*elf_section_callback_t
)(const char *name,
  const ElfW(Shdr) *section_header,
  const void *section_data,
  void *context);

/* Open an ELF file. Use the elf token for subsequent operations on the ELF. */
int elf_open(const char *path, elf_t *elf);

/* Open a memory resident ELF object. */
int elf_open_mem(void *mem, size_t size, elf_t *elf);

/* Close a previously opened ELF file. */
void elf_close(elf_t elf);

/* Get the ELF header. Note that the pointers returned are only valid for as
 * long as the ELF is opened. */
int elf_header(elf_t elf, const ElfW(Ehdr) **header);

/* Get section header and data. Note that the pointers returned are only valid
 * for as long as the ELF is opened. */
int elf_section(
    elf_t elf, const char *name, const ElfW(Shdr) **header, const void **data
);

/* Invoke the given callback on each ELF section. */
int elf_iterate_sections(
    elf_t elf, elf_section_callback_t callback, void *context
);

/* Get the value of a symbol from the ELF dynamic symbols table. */
int elf_symbol(elf_t elf, const char *symbol, ElfW(Addr) *addr);

#ifdef __cplusplus
}
#endif

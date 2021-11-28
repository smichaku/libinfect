#include <resolve.h>

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <elf.h>
#include <regex.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <util/compiler.h>
#include <util/common.h>
#include <util/elf.h>
#include <util/process.h>
#include <util/xerror.h>
#include <util/debug.h>

struct load_address_context {
    ino_t inode;
    uintptr_t start, end;
};

struct lib_soname_context {
    const regex_t *soname;
    char *path;
};

static int elf_soname(elf_t elf, const char **soname)
{
    int err;
    const void *section;
    const ElfW(Shdr) *dyn_hdr;
    const ElfW(Dyn) *dyn, *dyni;
    const ElfW(Shdr) *str_hdr;
    const char *strings;
    ElfW(Xword) offset;

    err = elf_section(elf, ".dynamic", &dyn_hdr, &section);
    if (err != 0) {
        return err;
    }

    dyn = section;
    for (dyni = dyn; dyni->d_tag != DT_NULL; dyni++) {
        if (dyni->d_tag == DT_SONAME) {
            offset = dyni->d_un.d_val;
            break;
        }
    }
    if (dyni->d_tag == DT_NULL) {
        *soname = NULL;
        return 0;
    }

    err = elf_section(elf, ".dynstr", &str_hdr, &section);
    if (err != 0) {
        return err;
    }

    strings = section;
    *soname = strings + offset;

    return 0;
}

static int check_lib_soname(
    const char *path, const regex_t *soname, bool *result
)
{
    int err, ret = 0;
    elf_t elf = NULL;
    const char *lib_soname;
    regmatch_t match;

    *result = false;

    if (path[0] == '\0') {
        goto out;
    }

    err = elf_open(path, &elf);
    if (err != 0) {
        if (err == XERR_BAD_FORMAT) {
            /* Not an ELF - not libc. */
            goto out;
        }

        ret = err;
        goto out;
    }

    err = elf_soname(elf, &lib_soname);
    if (err != 0) {
        ret = err;
        goto out;
    }

    if (lib_soname != NULL) {
        *result = regexec(soname, lib_soname, 1, &match, 0) == 0;
    } else {
        *result = false;
    }

out:
    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

static int check_soname_callback(const process_map_info_t *info, void *context)
{
    int err;
    struct lib_soname_context *lsc = context;
    bool match;

    if (lsc->path[0] != '\0') {
        return 0;
    }

    if (info->inode == 0 || !(info->prot & PROT_EXEC)) {
        return 0;
    }

    err = check_lib_soname(info->path, lsc->soname, &match);
    if (err != 0) {
        return err;
    }

    if (match) {
        strcpy(lsc->path, info->path);
    }

    return 0;
}

static int find_soname_path(pid_t pid, const char *pattern, char *path)
{
    int err, ret = 0;
    regex_t soname;
    struct lib_soname_context lsc;
    bool free_regex = false;

    err = regcomp(&soname, pattern, 0);
    if (err != 0) {
        ret = xerror(XECRegEx, err);
        goto out;
    }
    free_regex = true;

    lsc.soname = &soname;
    lsc.path = path;
    lsc.path[0] = '\0';

    err = iterate_maps(pid, check_soname_callback, &lsc);
    if (err != 0) {
        ret = err;
        goto out;
    }

    ret = lsc.path[0] != '\0' ? 0 : XERR(NOT_FOUND);

out:
    if (free_regex) {
        regfree(&soname);
    }

    return ret;
}

static int find_libc_path(pid_t pid, char *path)
{
    return find_soname_path(pid, "libc\\.so\\.[06]", path);
}

static int find_libdl_path(pid_t pid, char *path)
{
    return find_soname_path(pid, "libdl\\.so\\.[02]", path);
}

static int find_load_address_callback(
    const process_map_info_t *info, void *context
)
{
    struct load_address_context *lac = context;

    if (info->inode == lac->inode) {
        if (lac->start == UINT_MAX) {
            lac->start = (uintptr_t) info->start;
        }
        lac->end = (uintptr_t) info->end;
    } else if (lac->start != UINT_MAX) {
        /* All interesting mapping chunks were found - stop. */
        return 1;
    }

    return 0;
}

static int find_load_address(
    const char *file, pid_t pid, uintptr_t *start, uintptr_t *end
)
{
    int err;
    struct load_address_context lac;
    struct stat st;

    err = stat(file, &st);
    if (err == -1) {
        return xerror(XECLibc, errno);
    }

    lac.inode = st.st_ino;
    lac.start = UINT_MAX;
    lac.end = UINT_MAX;
    err = iterate_maps(pid, find_load_address_callback, &lac);
    if (lac.start == UINT_MAX || lac.end == UINT_MAX) {
        return err != 0 ? err : XERR(NOT_FOUND);
    }

    *start = lac.start;
    *end = lac.end;

    return 0;
}

int resolve_remote_symbol(
    pid_t pid, const char *lib, const char *sym, uintptr_t *value
)
{
    int err, ret = 0;
    char libpath[PATH_MAX];
    uintptr_t start = 0, end = 0;
    elf_t elf = NULL;
    ElfW(Addr) address;

    debug_printf("Resolving symbol %s:%s in process %d\n", lib, sym, pid);

    if (strcmp(lib, "libc") == 0) {
        err = find_libc_path(pid, libpath);
    } else if (strcmp(lib, "libdl") == 0) {
        err = find_libdl_path(pid, libpath);
    } else {
        err = XERR(NOT_FOUND);
    }
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = find_load_address(libpath, pid, &start, &end);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = elf_open(libpath, &elf);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = elf_symbol(elf, sym, &address);
    if (err != 0) {
        ret = err;
        goto out;
    }

    *value = address + start;

    debug_printf(
        "Resolved symbol %s:%s in process %d to 0x%" PRIxPTR "\n",
        lib,
        sym,
        pid,
        *value
    );

out:
    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

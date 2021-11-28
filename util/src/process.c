#define _GNU_SOURCE
#include <util/process.h>

#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include <util/common.h>
#include <util/elf.h>
#include <util/xerror.h>

static int load_address_callback(
    struct dl_phdr_info *info, size_t size, void *data
)
{
    void **load_address = data;

    UNUSED(size);

    /* We're interested in the executable which is always the first on the
     * list and has an empty string for a name. */
    if (info->dlpi_name[0] == '\0') {
        *load_address = (void *) info->dlpi_addr;
    }

    return 1;
}

int get_section_info(
    const char *name, void **addr, size_t *size, ElfW(Shdr) * header
)
{
    int err, ret = 0;
    elf_t elf = NULL;
    const ElfW(Ehdr) * elf_hdr;
    const ElfW(Shdr) * section_hdr;
    void *section;

    if (name == NULL || addr == NULL || size == NULL || header == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    err = elf_open("/proc/self/exe", &elf);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = elf_header(elf, &elf_hdr);
    if (err != 0) {
        ret = err;
        goto out;
    }

    err = elf_section(elf, name, &section_hdr, NULL);
    if (err != 0) {
        ret = err;
        goto out;
    }

    section = (void *) section_hdr->sh_addr;
    if (elf_hdr->e_type == ET_DYN) {
        /* Fix address according to executable's load address. */
        void *load_address = NULL;

        dl_iterate_phdr(load_address_callback, &load_address);
        if (load_address == NULL) {
            ret = XERR(UNEXPECTED_ERROR);
            goto out;
        }

        section = (void *) ((char *) load_address + section_hdr->sh_addr);
    }

    *addr = section;
    *size = section_hdr->sh_size;
    memcpy(header, section_hdr, sizeof(*header));

out:
    if (elf != NULL) {
        elf_close(elf);
    }

    return ret;
}

int iterate_maps(pid_t pid, map_callback_t callback, void *context)
{
    int err, ret = 0;
    char maps_path[64];
    FILE *maps_file = NULL;
    char line[1024];

    if (callback == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    sprintf(maps_path, "/proc/%d/maps", pid);

    maps_file = fopen(maps_path, "r");
    if (maps_file == NULL) {
        ret = xerror(XECLibc, errno);
        goto out;
    }

    while (fgets(line, sizeof(line), maps_file) != NULL) {
        int scanned;
        process_map_info_t info;
        char perms[4];
        char path[PATH_MAX + 1] = "";

        /* Parse the maps line - we except 7 or 8 items as the last one (path)
         * is not available for certain maps. */
        scanned = sscanf(
            line,
            "%p-%p %4c %llx %x:%x %lu %s",
            &info.start,
            &info.end,
            perms,
            &info.offset,
            &info.dev_major,
            &info.dev_minor,
            &info.inode,
            path
        );
        if (scanned == EOF) {
            ret = xerror(XECLibc, errno);
            goto out;
        }
        if (scanned < 7) {
            ret = XERR(UNEXPECTED_ERROR);
            goto out;
        }

        info.prot = 0;
        if (perms[0] == 'r') {
            info.prot |= PROT_READ;
        }
        if (perms[1] == 'w') {
            info.prot |= PROT_WRITE;
        }
        if (perms[2] == 'x') {
            info.prot |= PROT_EXEC;
        }

        info.sharing = 0;
        switch (perms[3]) {
        case 'p':
            info.sharing |= MAP_PRIVATE;
            break;
        case 's':
            info.sharing |= MAP_SHARED;
            break;
        }

        info.path = path;

        err = callback(&info, context);
        if (err != 0) {
            ret = err;
            goto out;
        }
    }

out:
    if (maps_file != NULL) {
        fclose(maps_file);
    }

    return ret;
}

int spawn(char * const argv[], bool daemonize, pid_t *pid)
{
    int err, ret = 0;
    int pfds[2] = { -1, -1 };
    pid_t newpid = -1;
    ssize_t rd;
    int wstatus;

    if (argv == NULL || argv[0] == NULL || pid == NULL) {
        return XERR(INVALID_ARGUMENT);
    }

    /* Using a pipe with FD_CLOEXEC to signal the parent when the child either
     * did a successful execv() or terminated before that due to an error. */
    err = pipe2(pfds, O_CLOEXEC);
    if (err == -1) {
        ret = xerror(XECLibc, errno);
        goto out;
    }

    newpid = fork();
    if (newpid == (pid_t) -1) {
        ret = xerror(XECLibc, errno);
        goto out;
    }

    if (newpid == 0) {
        close(pfds[0]);
        pfds[0] = -1;

        if (daemonize) {
            err = daemon(1, 1);
            if (err == -1) {
                goto child_exit;
            }
        }

        execv(argv[0], argv);

child_exit:
        err = errno;
        RETRY_SYSCALL(write(pfds[1], &err, sizeof(err)));
        _exit(127);
    }

    close(pfds[1]);
    pfds[1] = -1;

    if (daemonize) {
        /* In daemon() the immediate child which was forked by the spawn()
         * caller forked and exited immediately so we can just wait for it
         * here. */
        RETRY_SYSCALL(waitpid(newpid, &wstatus, 0));
    }

    rd = RETRY_SYSCALL(read(pfds[0], &err, sizeof(err)));
    if (rd == sizeof(err)) {
        /* Wait for the aborting child to terminate unless it's a daemon in
         * which case we already waited on the immediate child and have no
         * relation to the daemon process. */
        if (!daemonize) {
            RETRY_SYSCALL(waitpid(newpid, &wstatus, 0));
        }

        ret = xerror(XECLibc, err);
    }

out:
    if (pfds[0] != -1) {
        close(pfds[0]);
    }
    if (pfds[1] != -1) {
        close(pfds[1]);
    }

    if (ret == 0) {
        *pid = daemonize ? 0 : newpid;
    }

    return ret;
}

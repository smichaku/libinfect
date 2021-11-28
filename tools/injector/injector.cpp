#include <sys/types.h>
#include <string>
#include <optional>
#include <iostream>

#include <util/xerror.h>
#include <util/elf.h>
#include <infect/inject.h>

#include "exceptions.hpp"

namespace {

class ELF final {
public:
    struct Section {
        const ElfW(Shdr) * header;
        const void *data;
    };

    explicit ELF(const std::string &path)
    {
        int err = elf_open(path.c_str(), &m_elf);
        if (err != 0) {
            throw InjectorException("Failed opening ELF", err);
        }
    }

    ELF() = delete;
    ELF(const ELF &) = delete;
    ELF(ELF &&) = delete;

    ~ELF() noexcept
    {
        elf_close(m_elf);
    }

    ELF &operator=(const ELF &) = delete;
    ELF &operator=(ELF &&) = delete;

    std::optional<Section> section(const std::string &name)
    {
        int err;

        Section section;
        err = elf_section(m_elf, name.c_str(), &section.header, &section.data);
        if (err != 0) {
            if (err == XERR_NOT_FOUND) {
                return {};
            }

            throw InjectorException("Unable to get ELF section", err);
        }

        return section;
    }

private:
    elf_t m_elf = nullptr;
};

bool is_module(const std::string &path)
{
    return ELF(path).section(".module_info").has_value();
}

} // namespace

void do_inject(pid_t pid, const std::string &path)
{
    int err;

    void *handle;
    if (is_module(path)) {
        std::cout << "Injecting module" << std::endl;
        err = inject_module(pid, path.c_str(), &handle);
    } else {
        std::cout << "Injecting DSO" << std::endl;
        err = inject_dso(pid, path.c_str(), &handle);
    }
    if (err != 0) {
        throw InjectorException("Injection failed", err);
    }

    std::cout << "Injection successful, handle = " << handle << std::endl;
}

void do_eject(pid_t pid, void *handle)
{
    int err;

    // Attempt ejecting the injectable as a module. If the handle is to a DSO
    // then eject_module() will return with XERR_BAD_FORMAT.
    err = eject_module(pid, handle);
    if (err != 0) {
        if (err == XERR_BAD_FORMAT) {
            err = eject_dso(pid, handle);
        }
    }
    if (err != 0) {
        throw InjectorException("Ejection failed", err);
    }

    std::cout << "Ejection successful" << std::endl;
}

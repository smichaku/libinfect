#include <sys/types.h>
#include <optional>
#include <string>

#include <CLI11.hpp>

extern void do_inject(pid_t pid, const std::string &path);
extern void do_eject(pid_t pid, void *handle);


int main(int argc, const char **argv)
{
    CLI::App app{ "injector" };

    std::optional<pid_t> pid;
    std::optional<std::string> path;
    std::optional<void *> handle;

    app.require_subcommand(1);

    CLI::App *inject = app.add_subcommand("inject", "Inject a DSO or module");
    inject->add_option("--pid", pid, "PID of the process to inject into")
        ->required();
    inject->add_option("--path", path, "Path to injectable")->required();

    CLI::App *eject = app.add_subcommand("eject", "Eject a DSO or module");
    eject->add_option("--pid", pid, "PID of the process to inject into")
        ->required();
    eject->add_option("--handle", handle, "Handle of injectable")->required();

    CLI11_PARSE(app, argc, argv);

    if (*inject) {
        do_inject(*pid, *path);
    }
    if (*eject) {
        do_eject(*pid, *handle);
    }
}

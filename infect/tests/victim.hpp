#pragma once

#include <cstdlib>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <util/common.h>
#include <util/process.h>

#include <catch.hpp>

extern "C" void victim(void);

class Victim final {
public:
    Victim() : pid(spawn())
    {
    }

    ~Victim()
    {
        int werr;

        do {
            kill(pid, SIGUSR1);
            werr = RETRY_SYSCALL(waitpid(pid, nullptr, WNOHANG));
        } while (werr == 0);
    }

    const pid_t pid;

private:
    static pid_t spawn()
    {
        pid_t pid = fork();
        if (pid == -1) {
            FAIL("Failed forking victim");
        }

        if (pid == 0) {
            victim();
            _exit(0);
        }

        return pid;
    }
};

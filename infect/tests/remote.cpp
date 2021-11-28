extern "C" {
#include <remote.h>
}

#include <cstddef>
#include <cstdint>
#include <cstring>

#include <catch.hpp>
#include <finally.hpp>

#include <util/xerror.h>

#include "victim.hpp"

#define TEST_CASE_TAG "[remote]"

extern "C" ssize_t __real_process_vm_readv(
    pid_t pid,
    const struct iovec *local_iov,
    unsigned long liovcnt,
    const struct iovec *remote_iov,
    unsigned long riovcnt,
    unsigned long flags
);

static bool emulate_no_process_vm = false;

extern "C" ssize_t __wrap_process_vm_readv(
    pid_t pid,
    const struct iovec *local_iov,
    unsigned long liovcnt,
    const struct iovec *remote_iov,
    unsigned long riovcnt,
    unsigned long flags
)
{
    if (emulate_no_process_vm) {
        errno = ENOSYS;
        return -1;
    }

    return __real_process_vm_readv(
        pid, local_iov, liovcnt, remote_iov, riovcnt, flags
    );
}

TEST_CASE("remote_open/close()", TEST_CASE_TAG)
{
    int err;
    struct remote *remote = nullptr;

    Victim victim;

    err = remote_open(victim.pid, &remote, 0);
    REQUIRE(err == 0);

    auto remote_cleanup = finally([&] { REQUIRE(remote_close(remote) == 0); });

    REQUIRE(remote != nullptr);
    REQUIRE(remote->pid == victim.pid);
    REQUIRE(!remote->terminated);
}

TEST_CASE("remote_read()", TEST_CASE_TAG)
{
    int err;
    struct remote *remote = nullptr;

    Victim victim;

    err = remote_open(victim.pid, &remote, 0);
    REQUIRE(err == 0);
    auto remote_cleanup = finally([&] { REQUIRE(remote_close(remote) == 0); });

    static const uint8_t TestBlock[] = { 0xfe, 0x89, 0xe2, 0x44,
                                         0x21, 0x9b, 0x8f, 0xf1 };

    std::uint8_t buffer[sizeof(TestBlock)] = { 0 };

    // WB-TEST: Check that the ptrace-based memory read function works property.
    emulate_no_process_vm = GENERATE(false, true);
    auto emulate_cleanup = finally([] { emulate_no_process_vm = false; });

    err = remote_read(
        remote, reinterpret_cast<uintptr_t>(TestBlock), buffer, sizeof(buffer)
    );
    REQUIRE(err == 0);
    REQUIRE(memcmp(buffer, TestBlock, sizeof(TestBlock)) == 0);
}

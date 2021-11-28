#include <cstdint>
#include <cstdlib>
#include <string>

#include <catch.hpp>
#include <xcatch.hpp>

#include <util/xerror.h>
#include <infect/inject.h>

#include "victim.hpp"

#define TEST_CASE_TAG "[inject]"

TEST_CASE("inject_code()", TEST_CASE_TAG)
{
    int err;

    Victim victim;

    void *handle;
    SECTION("Non terminating code")
    {
        // clang-format off
        static const uint8_t Code[] = {
#if defined(__x86_64__)
            0x48, 0x01, 0xf7,           /* add %rsi, %rdi */
            0x48, 0x89, 0xf8,           /* mov %rdi, %rax */
            0xc3,                       /* ret */
#elif defined(__arm__)
            0x00, 0x00, 0x81, 0xe0,     /* add r0, r1, r0 */
            0x1e, 0xff, 0x2f, 0xe1,     /* bx lr */
#endif
        };
        // clang-format on

        err = inject_code(victim.pid, Code, sizeof(Code), 0, &handle);
        REQUIRE_NO_XERR(err);

        err = eject_code(victim.pid, handle);
        REQUIRE_NO_XERR(err);
    }
    SECTION("Terminating code")
    {
        // clang-format off
        static const uint8_t Code[] = {
#if defined(__x86_64__)
            0x48, 0xC7, 0xC0, 0x3C, 0x00, 0x00, 0x00,   /* mov $0x3c, %rax */
            0x48, 0xC7, 0xC7, 0x01, 0x00, 0x00, 0x00,   /* mov $1, %rdi */
            0x0f, 0x05,                                 /* syscall */
#elif defined(__arm__)
            0x01, 0x70, 0xa0, 0xe3,                     /* mov r7, #1 */
            0x00, 0x00, 0x00, 0xef,                     /* svc #0 */
#endif
        };
        // clang-format on

        err = inject_code(victim.pid, Code, sizeof(Code), 0, &handle);
        REQUIRE(err == XERR_TERMINATED);
    }
}

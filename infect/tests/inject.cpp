#include <cstdlib>
#include <string>
#include <tuple>

#include <filesystem.hpp>
#include <catch.hpp>
#include <xcatch.hpp>

#include <util/common.h>
#include <util/process.h>
#include <util/embeddable.h>
#include <infect/inject.h>

#include "victim.hpp"

#define TEST_CASE_TAG "[inject]"

namespace fs = filesystem;

namespace {

std::string injectable_path(const char *name)
{
    return fs::current_path() / std::string(name);
}

void unload_injectable(const char *name)
{
    int err;

    err = embeddable_unload(name, injectable_path(name).c_str(), 0660);
    if (err != 0) {
        FAIL("Unable to unload injectable");
    }
}

} // namespace

struct SafeDSOInject {
    constexpr static auto Injectable = "infect.tests.dso.so";
    constexpr static auto inject = inject_dso;
    constexpr static auto eject = eject_dso;
};

struct UnsafeDSOInject {
    constexpr static auto Injectable = "infect.tests.dso.so";
    constexpr static auto inject = inject_dso_unsafe;
    constexpr static auto eject = eject_dso_unsafe;
};

struct ModuleInject {
    constexpr static auto Injectable = "infect.tests.module.so";
    constexpr static auto inject = inject_module;
    constexpr static auto eject = eject_module;
};

TEMPLATE_TEST_CASE(
    "inject()", TEST_CASE_TAG, SafeDSOInject, UnsafeDSOInject, ModuleInject
)
{
    int err;

    Victim victim;

    unload_injectable(TestType::Injectable);

    const fs::path TestFile{ "inject-ok" };

    fs::remove(TestFile);

    auto inject = TestType::inject;
    auto eject = TestType::eject;

    void *handle;
    err = inject(
        victim.pid, injectable_path(TestType::Injectable).c_str(), &handle
    );
    REQUIRE_NO_XERR(err);
    REQUIRE(handle != nullptr);
    REQUIRE(fs::exists(TestFile));
    REQUIRE(fs::file_size(TestFile) == 1);

    fs::remove(TestFile);

    err = eject(victim.pid, handle);
    REQUIRE_NO_XERR(err);
}

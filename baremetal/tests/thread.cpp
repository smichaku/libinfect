#include <stdint.h>
#include <sys/user.h>

#include <catch.hpp>

extern "C" {
#include <baremetal/mman.h>
#include <baremetal/thread.h>
}

#define TEST_CASE_TAG "[thread]"

static size_t pagesize()
{
    int err;
    size_t pgsize;

    err = bm_pagesize(&pgsize);
    if (err != 0) {
        FAIL("Unable to get page size\n");
    }

    return pgsize;
}

TEST_CASE("thread_create/join()", TEST_CASE_TAG)
{
    int err;
    thread_t thread;
    void *rv;

    err = bm_thread_create(
        [](void *x) { return (void *) ((uintptr_t) x + 1); },
        (void *) 38,
        pagesize(),
        &thread
    );
    REQUIRE(err == 0);

    err = bm_thread_join(thread, &rv);
    REQUIRE(err == 0);
    REQUIRE((uintptr_t) rv == 39);
}

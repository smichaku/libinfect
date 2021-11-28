#include <util/xerror.h>

#include <string.h>

#include <util/common.h>

static const char * const XErrorStrings[] = {
#define X(arg) #arg,
#include <util/error_codes.inc>
#undef X
};

#if __has_include(<execinfo.h>)
#include <execinfo.h>
#define HAVE_BACKTRACE
#endif

struct backtrace {
    void *frames[XERROR_BACKTRACE_MAXLEN];
    unsigned int count;
};

static __thread struct backtrace g_xerror_backtrace = { 0 };

int xerror(xerror_class_t cls, unsigned int id)
{
#ifdef HAVE_BACKTRACE
    g_xerror_backtrace.count = backtrace(
        g_xerror_backtrace.frames, ARRAY_SIZE(g_xerror_backtrace.frames)
    );
#endif

    return xerror_code(cls, id);
}

const char *xerror_string(int err, char *string, size_t size)
{
    const xerror_class_t cls = xec(err);
    const unsigned int id = xeid(err);
    char *ptr;
    size_t length;

    if (string == NULL || size == 0) {
        return NULL;
    }

    ptr = string;
    length = string + size - ptr - 1;

    switch (cls) {
    case XECInfect:
        if (id < ARRAY_SIZE(XErrorStrings)) {
            snprintf(ptr, length, "libinfect error - %s", XErrorStrings[id]);
        } else {
            snprintf(ptr, length, "libinfect error - UNKNOWN ERROR");
        }
        break;
    case XECLibc:
        ptr = stpncpy(ptr, "libc error - ", length);
        length = string + size - ptr - 1;
        strerror_r(id, ptr, length);
        break;
    case XECPThread:
        ptr = stpncpy(ptr, "pthread error - ", length);
        length = string + size - ptr - 1;
        strerror_r(id, ptr, length);
        break;
    case XECLibDL:
        ptr = stpncpy(ptr, "libdl error", length);
        length = string + size - ptr - 1;
        break;
    case XECRegEx:
        snprintf(ptr, length, "regex error - %d", id);
        break;
    default:
        snprintf(ptr, length, "unknown error %d (0x%x)", err, err);
        break;
    }

    string[size - 1] = '\0';
    return string;
}

int xerror_backtrace(void **buffer, unsigned int capacity, unsigned int *count)
{
    if (buffer == NULL || capacity == 0 || count == NULL) {
        return XERR_INVALID_ARGUMENT;
    }

#ifdef HAVE_BACKTRACE
    if (g_xerror_backtrace.count > capacity) {
        return XERR_BUFFER_TOO_SMALL;
    }

    memcpy(
        buffer,
        g_xerror_backtrace.frames,
        g_xerror_backtrace.count * sizeof(void *)
    );

    *count = g_xerror_backtrace.count;

    return 0;
#else
    return XERR_NOT_IMPLEMENTED;
#endif
}

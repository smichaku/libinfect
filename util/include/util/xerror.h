#pragma once

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "error_codes.h"

#define XERROR_BACKTRACE_MAXLEN 128

/* Error codes consist of a class identifying the component to which the error
 * is related and a specific error identifier in that component.
 *
 * The error class resides in the MSB while the specific error identifier
 * resides in the rest of the int value and must be a positive integer.
 */

#define _XEC_SHIFT ((sizeof(int) - 1) * CHAR_BIT)

/* Macro for generating a generic error code (class code 0). This macro invokes
 * the xerror() function which generates and overwrites the xerror backtrace so
 * one should avoid using it when testing an existing code against a value and
 * use the XERR_XXX form instead:
 *
 * err = foo();
 * if (err == XERR(INVALID_ARGUMENT))   // Bad - a new backtrace is generated
 * if (err == XERR_INVALID_ARGUMENT)    // Good - no side-effects
 */
#define XERR(err) xerror(XECInfect, XERR_##err)

#define XERRCODE(err) xerror_code(XECInfect, XERR_##err)

typedef enum
{
    /* The class for libinfect errors is 0 so we don't need to call xerror()
     * unless we want to report an error from a different component. */
    XECInfect = 0x00,

    /* System / libc errno style errors. */
    XECLibc = 0x10,

    /* PThread errors. */
    XECPThread = 0x11,

    /* libdl errors. */
    XECLibDL = 0x12,

    /* POSIX regex errors. */
    XECRegEx = 0x13,
} xerror_class_t;

/**
 * Generates an error code for a specific class (component). This function
 * generates a backtrace as a side effect so one should only call it when the
 * error is to be generated and use the xec(), xeid() and xerror_code() for
 * checking an existing xerror code.
 *
 * err = foo();
 * if (err == xerror(XECLibc, ENOENT))      // Bad - a new backtrace is
 * generated. if (xec(err) == XECLibc &&               // Good - no side effect.
 *     xeid(err) == ENOENT)
 * if (err == xerror_code(XECLibc, ENOENT)) // Good - no side effect.
 */
int xerror(xerror_class_t cls, unsigned int id);

/**
 * Generates an error code without additional side-effects.
 */
static inline int xerror_code(xerror_class_t cls, unsigned int id)
{
    assert((int) cls <= UCHAR_MAX);
    assert(id > 0);
    assert(id < (1U << _XEC_SHIFT));

    return (cls << _XEC_SHIFT) | id;
}

/* Get the error class from the given error code. */
static inline xerror_class_t xec(int err)
{
    return (xerror_class_t) ((unsigned) err >> _XEC_SHIFT);
}

/* Get the error id from the given error code. */
static inline unsigned int xeid(int err)
{
    return (unsigned) err & ((1U << _XEC_SHIFT) - 1);
}

/**
 * Returns a string representation for an error code.
 *
 * @err - The xerror code.
 * @string - A buffer for the formatted error string.
 * @size - Size of the `string` buffer in bytes.
 *
 * Returns `string` if successful or NULL otherwise.
 */
const char *xerror_string(int err, char *string, size_t size);

/*
 * Get a backtrace of where the last xerror was generated.
 *
 * @buffer - An array of the backtrace return-addresses.
 * @capacity - Maximum number of pointers in 'buffer'.
 * @count - The number of frames returned in 'buffer'.
 */
int xerror_backtrace(void **buffer, unsigned int capacity, unsigned int *count);

#ifdef __cplusplus
}
#endif

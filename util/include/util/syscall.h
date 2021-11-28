#pragma once

/* Check whether the value returned by a syscall represents an error code - if
 * so the returned value is the errno value for that error, otherwise 0 is
 * returned. */
static inline int syscall_error(long lr)
{
    unsigned long ulr = lr;

    if (ulr > -4096UL) {
        return -ulr;
    }

    return 0;
}

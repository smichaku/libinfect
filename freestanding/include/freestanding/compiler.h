#pragma once

#define unused       __attribute__((unused))
#define maybe_unused unused

#define weak_alias(old, new)                                                   \
    extern __typeof(old) new __attribute__((__weak__, __alias__(#old)))

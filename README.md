libinfect
=========

[![Build Status](https://github.com/smichaku/libinfect/actions/workflows/build.yml/badge.svg)](https://github.com/smichaku/libinfect/actions/workflows/build.yml "Build Statues")
[![Cross-build Status](https://github.com/smichaku/libinfect/actions/workflows/cross-build.yml/badge.svg)](https://github.com/smichaku/libinfect/actions/workflows/cross-build.yml "Cross-build Statues")


A remote process code injection library for Linux

Purpose and features
--------------------

libinfect provides the following APIs for injecting code into a remote process:

  * `inject/eject_code()` - For injecting/ejecting shellcode.

  * `inject/eject_dso()` - For injecting/ejecting a shared object which has
    dynamic dependencies on runtime libraries (e.g. libc).

  * `inject/eject_module()` - For injecting/ejecting a shared object which has no
    dynamic dependencies.

The DSO API relies on the dynamic loader in the remote process to satisfy all
the dynamic requirements of the injected object. Therefore, this API will fail
in case of a missing library or symbol or if the process' executable has
static linkage.

The module API is for injecting an independent shared object without relying
on dynamic linkage support in the remote process. As modules may not rely on
standard system libraries (such as libc) they must be self-contained and
provide all required functionality on their own. The libinfect project
provides a small set of libc function and syscall wrappers in the
libbaremetal library as well as a CMake module (InfectModule) for adding
modules. For more information, please see [How to write you own libinfect
module](doc/module.md).

libinfect currently supports the x86_64 and ARM32 architectures. The target
architecture is auto-detected if a toolchain file is passed to CMake.

Building and installing
-----------------------

Use CMake for building and installing:

```
cmake -B build -S .
cmake --build build
cmake --install build
```

Usage
-----

Injecting code:

```
#include <infect/inject.h>

...

uint8_t code[] = { ... };

/* Inject the code and get a handle for ejecting it later. */
void *handle;
err = inject_code(pid, code, sizeof(code), 0, &handle); 
if (err != 0) {
  ...
} 

...

/* Eject the previously injected code. */
eject_code(pid, handle);
```

Injecting a DSO or a module:

```
#include <inject/inject.h>

...

const char *path = "/tmp/injectable.so";

/* Inject the DSO and get a handle for ejecting it later. */
void *handle;
err = inject_dso(pid, path, &handle);
if (err != 0) {
  ...
}

...

/* Eject the previously injected DSO. */
err = eject_dso(pid, handle);
```

Limitations
-----------

libinfect relies on the calling process having sufficient privileges for tracing
(ptrace(2)) the remote process. This means the remote process must either be a
child of the calling process or that the process has the CAP_SYS_PTRACE
capability.

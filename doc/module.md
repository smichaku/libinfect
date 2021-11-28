How to write your own libinfect module
======================================

libinfect modules are shared objects which have no dynamic runtime dependency.
Building an injectable as a module instead of a DSO is beneficial when the
runtime environment for the remote process is unknown (i.e. we can't rely on the
existence of certain shared libraries or symbols within these libraries) or when
the remote process does not support dynamic linkage at all (e.g. statically
linked executable).

Modules may not use libc or any other standard system library and must be
self-contained. Statically linking the module with libc may be possible in some
situations but will probably fail the dynamic linkage if the libc in the
process' runtime is different than the one linked with the module. To fill the
gap libbaremetal provides both libinfect and client-implemented modules with
a subset of libc functions for general use.

The libinfect package provides the `add_infect_module()` CMake function for
adding a module:

```
Project(...)

find_package(Infect REQUIRED CONFIG)

add_infect_module(my-module
  my-module.c
)
```

The module will be compiled and linked with the required flags. It may then be
injected by libinfect by using the `inject_modue()` API.

.section .rodata

.global async_dlfcn_start
.global async_dlfcn_end

async_dlfcn_start: .incbin "async_dlfcn.so"
async_dlfcn_end:

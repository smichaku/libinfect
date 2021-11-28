#pragma once

enum libinfect_error
{
#define X(arg) XERR_##arg,
#include "error_codes.inc"
#undef X
};

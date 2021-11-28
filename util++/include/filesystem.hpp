#pragma once

#if __has_include(<version>)
#include <version>
#elif __has_include(<experimental/filesystem>)
#define __cpp_lib_experimental_filesystem
#endif

#if defined(__cpp_lib_filesystem)
#include <filesystem>
namespace filesystem = std::filesystem;
#elif defined(__cpp_lib_experimental_filesystem)
#include <experimental/filesystem>
namespace filesystem = std::experimental::filesystem;
#endif

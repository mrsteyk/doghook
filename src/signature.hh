#pragma once

#include <cassert>

#include "platform.hh"

namespace Signature {
void *resolve_library(const char *name);
void *resolve_import(void *handle, const char *name);

u8 *find_pattern(const char *module, const char *pattern);
u8 *find_pattern(const char *pattern, uptr start, uptr length);

template <typename T>
auto find_pattern(const char *module, const char *pattern, u32 offset) {
    return reinterpret_cast<T>(find_pattern(module, pattern) + offset);
}
void *resolve_callgate(void *address);

// TODO: there must be some way we can do this at init time instead of using magic statics.
// make a static chain and find them at level_init or at init_all
class Pattern {
    const char *module;
    const char *signature;

public:
    Pattern(const char *                 module,
            [[maybe_unused]] const char *signature_windows,
            [[maybe_unused]] const char *signature_linux,
            [[maybe_unused]] const char *signature_osx)
        : module(module),
#if doghook_platform_windows()
          signature(signature_windows)
#elif doghook_platform_linux()
          signature(signature_linux)
#elif doghook_platform_osx()
          signature(signature_osx)
#endif
    {
        assert(signature[0] != '\0');
    }

    auto find() -> u8 * {
        return ::Signature::find_pattern(module, signature);
    }

    template <typename T>
    auto find(u32 offset) -> T {
        return ::Signature::find_pattern<T>(module, signature, offset);
    }
};
} // namespace Signature

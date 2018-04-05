#pragma once

#include "platform.hh"

// This file defines helpers and functionality for using and updating classids

namespace sdk {
namespace class_id {

// See comment blow for #define ID
namespace internal_checker {
class ClassIDChecker {
    static ClassIDChecker *head;

public:
    const char *    name;
    u32             intended_value;
    ClassIDChecker *next = nullptr;

    ClassIDChecker(const char *name, const u32 value);

    bool        check_correct();
    static void check_all_correct();
};
} // namespace internal_checker

// For debug builds we want to be able to check our classids are correct and issue warnings if they are not correct
// So that we can update the value for next time.
#if defined(_DEBUG) && defined(PLACE_CHECKER)
#define ID(name, value)                          \
    enum { name = value };                       \
    namespace internal_checker {                 \
    inline auto checker_##name = ClassIDChecker( \
        #name,                                   \
        value);                                  \
    }
#else
#define ID(name, value) \
    enum { name = value };
#endif

// Put ids here
ID(CTFPlayer, 246);
ID(CTFRevolver, 285);
ID(CTFSniperRifle, 306);
ID(CTFFlareGun, 208);

#undef ID

} // namespace class_id
} // namespace sdk

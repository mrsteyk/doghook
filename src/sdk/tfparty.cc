#include <precompiled.hh>

#include <sdk/signature.hh>

namespace tf {
namespace party {
static uptr thisptr{NULL};

void get_tf_party() {
#if doghook_platform_windows()
    thisptr = *signature::find_pattern<uptr *>("client", "A1 ? ? ? ? C3", 1);
#elif doghook_platform_linux()
    thisptr         = *signature::find_pattern<u32 *>("client", "55 A1 ? ? ? ? 89 E5 5D C3 8D B6 00 00 00 00 A1 ? ? ? ? 85 C0", 2);
#else
    //todo mac
#endif
}

bool in_queue() {
    return *(u8 *)(thisptr + 69);
}

void request_queue(int type) {
    typedef char(
#if doghook_platform_windows()
        __thiscall
#endif
        * func)(uptr, int);
#if doghook_platform_windows()
    static func kek = signature::find_pattern<func>("client", "55 8B EC 83 EC 38 56 8B 75 08 57", 0);
#elif doghook_platform_linux()
    static func kek = signature::find_pattern<func>("client", "55 89 E5 57 56 53 81 EC ? ? ? ? 8B 75 ? 89 F0", 0);
#else
    static func kek{NULL};
#endif
    kek(thisptr, type);
}

void request_leave_queue(int type) {
    //55 8B EC 83 EC 18 53 56 8B 75 08 8B D9 56
    typedef bool(
#if doghook_platform_windows()
        __thiscall
#endif
        * func)(uptr, int);
#if doghook_platform_windows()
    static func kek = signature::find_pattern<func>("client", "55 8B EC 83 EC 18 53 56 8B 75 08 8B D9 56", 0);
#elif doghook_platform_linux()
    static func kek = signature::find_pattern<func>("client", "55 89 E5 57 56 53 83 EC ? 8B 45 ? 89 44 24 ? 8B 45 ? 89 04 24 E8 ? ? ? ? 84 C0 89 C6 75 ?", 0);
#else
    static func kek{NULL};
#endif

	kek(thisptr, type);
}

} // namespace party
namespace gc {
static uptr thisptr{NULL}; // B8 ? ? ? ? C3

void get_gc() {
    uptr tmp{NULL};
#if doghook_platform_windows()
    tmp = signature::find_pattern<uptr>("client", "B8 ? ? ? ? C3", 1);
#elif doghook_platform_linux()
    tmp             = signature::find_pattern<uptr>("client", "55 B8 ? ? ? ? 89 E5 5D C3 8D B6 00 00 00 00 55 A1 ? ? ? ? 89 E5 5D C3 8D B6 00 00 00 00 A1 ? ? ? ?", 2);
#else
    //todo mac;
#endif
    thisptr = tmp + *(uptr *)tmp;
}

bool is_connect_to_match_server() {
    return (u32)(*(unsigned long *)(thisptr + 1332) - 1) <= 1;
}

void abandon() {
    typedef int (*func)(uptr);
#if doghook_platform_windows()
    static func kek = signature::find_pattern<func>("client", "55 8B EC 83 EC 14 56 57 8B 3D ? ? ? ?", 0);
#elif doghook_platform_linux()
    static func kek = signature::find_pattern<func>("client", "55 89 E5 57 56 8D 75 ? 53 81 EC ? ? ? ? C7 04 24 ? ? ? ?", 0);
#else
    // todomac
    static func kek{NULL};
#endif
    kek(thisptr);
}

} // namespace gc

} // namespace tf

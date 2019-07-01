#include <precompiled.hh>

#include <sdk/hooks.hh>
#include <sdk/sdk.hh>

using namespace sdk;

namespace write_ucmd2buf {

std::unique_ptr<hooks::HookFunction<Client, 0>> write_ucmd2buf_hook;

// I don't remember why i did it tho???

#if doghook_platform_windows()
u8 __fastcall hooked_write_ucmd2buf(Client *clnt, [[maybe_unused]] void *, uptr buf, u32 from, u32 to, bool isnewcommand)
#else
u8 hooked_write_ucmd2buf(Client *clnt, uptr buf, u32 from, u32 to, bool isnewcommand)
#endif
{
#if doghook_platform_windows()
    using write_ucmd2buf_t        = void(__cdecl *)(uptr, UserCmd *, UserCmd *);
    static auto write_ucmd2buf_fn = (write_ucmd2buf_t)signature::resolve_callgate(signature::find_pattern("client", "E8 ? ? ? ? 83 C4 0C 80 7B 10 00"));
#else
#error "Implement me!"
#endif
    UserCmd nullcmd, *f, *t;

    if (from == -1) {
        f = &nullcmd;
    } else {
        f = (UserCmd *)iface::input->get_verified_user_cmd(from);

        if (!f)
            f = &nullcmd;
    }

    t = (UserCmd *)iface::input->get_verified_user_cmd(to);

    if (!t)
        t = &nullcmd;

    write_ucmd2buf_fn(buf, t, f);

    if (*(u8 *)(buf + 0x10)) //IsOverflowed
        return false;

    return true;
}

void level_init() {
    assert(write_ucmd2buf_hook == nullptr);
    write_ucmd2buf_hook = std::make_unique<hooks::HookFunction<Client, 0>>(iface::client, 23, -1, -1, reinterpret_cast<void *>(&hooked_write_ucmd2buf));
}

void level_shutdown() {
    write_ucmd2buf_hook.reset();
    write_ucmd2buf_hook = nullptr;
}
} // namespace write_ucmd2buf

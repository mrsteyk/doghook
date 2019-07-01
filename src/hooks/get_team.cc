#include <precompiled.hh>

#include <sdk/hooks.hh>
#include <sdk/log.hh>
#include <sdk/player.hh>
#include <sdk/sdk.hh>

#if doghook_platform_msvc()
#include <intrin.h>
#pragma intrinsic(_ReturnAddress)
#endif

using namespace sdk;

class CTFPlayerResource {
public:
    u32 get_team(u32 idx) {
#if doghook_platform_windows()
        // offset is not needed cuz we only call it from where thisptr is moved already???
        return_virtual_func(get_team, 13, -1, -1, 340, idx);
#else
#error Implement me!
#endif
    }
};

namespace get_team {

#if doghook_platform_windows()
std::unique_ptr<hooks::HookFunction<CTFPlayerResource, 340>> get_team_scoreboard_hook;
std::unique_ptr<hooks::HookFunction<CTFPlayerResource, 340>> get_team_playerpanel_hook;
#else
#error "Implement me!"
#endif

u8 *retaddr_2nd_layer_playerpanel = nullptr;
//u32 __fastcall hooked_get_team_fn(CTFPlayerResource *pr, [[maybe_unused]] void *, u32 idx);

// i didnt want to release it, but still :-( i expect this shit to be in every """hack""" now without crediting me :-(((((
u32 __declspec(naked) __fastcall hooked_get_team_playerpanel(CTFPlayerResource *pr, [[maybe_unused]] void *, u32 idx) {
    __asm {
		push ebp;
		mov ebp,esp;
    }
    get_team_playerpanel_hook->call_original<u32>(idx);
    __asm {
		push ecx;
		mov ecx, retaddr_2nd_layer_playerpanel;
		cmp [ebp+0xC], ecx;
		pop ecx;
		jne kek;
		mov esi, eax;
	kek:
		pop ebp;
		retn 4;
    }
}

// this shits to gonna be in like every """hack""" without any credit
#if doghook_platform_windows()
u32 __fastcall hooked_get_team_scoreboard(CTFPlayerResource *pr, [[maybe_unused]] void *, u32 idx)
#else
u32 hooked_get_team(CTFPlayerResource *pr, u32 idx)
#endif
{
    auto ret = get_team_scoreboard_hook->call_original<u32>(idx);

    auto lp = Player::local();
    if (!lp->is_valid() || lp->team() < 2)
        return ret;

#if doghook_platform_windows()
    static auto retaddr_scoreboard  = signature::find_pattern("client", "FF 50 34 83 F8 02 75 49") + 3;
    static auto retaddr_playerpanel = signature::find_pattern("client", "FF 50 34 C3") + 3;
#else
#error Implement me!
#endif
#if doghook_platform_msvc()
    if (_ReturnAddress() == retaddr_scoreboard) {
        return lp->team();
    }
#else
#error Implement me!
#endif

    return ret;
}

void level_init() {
    assert(get_team_scoreboard_hook == nullptr);
    assert(get_team_playerpanel_hook == nullptr);
    // TODO: find out how its done on linux!
#if doghook_platform_windows()
    retaddr_2nd_layer_playerpanel = signature::find_pattern("client", "3B C6 0F 94 45 FF");

    static auto g_pr                = **(CTFPlayerResource ***)(signature::find_pattern("client", "FF 35 ? ? ? ? 83 CB FF") + 2);
    static auto playerresource_dang = **(CTFPlayerResource ***)(signature::find_pattern("client", "74 6C 8B 0D ? ? ? ? 81 C1") + 4);
    get_team_scoreboard_hook        = std::make_unique<hooks::HookFunction<CTFPlayerResource, 340>>(g_pr, 13, -1, -1, reinterpret_cast<void *>(&hooked_get_team_scoreboard));
    get_team_playerpanel_hook       = std::make_unique<hooks::HookFunction<CTFPlayerResource, 340>>(playerresource_dang, 13, -1, -1, reinterpret_cast<void *>(&hooked_get_team_playerpanel));
#else
#error Implement me!
#endif
}

void level_shutdown() {
    get_team_scoreboard_hook.reset();
    get_team_scoreboard_hook = nullptr;
    get_team_playerpanel_hook.reset();
    get_team_playerpanel_hook = nullptr;
}
} // namespace get_team

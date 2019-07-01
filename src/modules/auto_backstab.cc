#include <precompiled.hh>

#include <sdk/platform.hh>

#include <sdk/class_id.hh>
#include <sdk/log.hh>
#include <sdk/player.hh>
#include <sdk/sdk.hh>
#include <sdk/trace.hh>
#include <sdk/weapon.hh>
#include <utils/profiler.hh>

#include <modules/backtrack.hh>

using namespace sdk;

// this bs barely works for me, idk what fucks it

namespace auto_backstab {
bool legit_check() {
    profiler_profile_function();

    //DoSwingTrace(trace) = [523]
#if doghook_platform_linux()                                                                              // TODO: make it work on linux!
    using is_behind_and_facing_target_fn    = bool(__cdecl *)(sdk::Player * owner, sdk::Player * target); // may be same as windows, but i forgot and don't have binaries rn
    static auto is_behind_and_facing_target = signature::find_pattern<is_behind_and_facing_target_fn>("client", "55 89 E5 57 56 53 83 EC ? 8B 45 ? 8B 75 ? 89 04 24 E8 ? ? ? ? 85 C0 89 C3 74", 0);
#error "Reverse some more!"
#elif doghook_platform_windows()
    using is_behind_and_facing_target_fn              = bool(__thiscall *)(sdk::Weapon * weapon, sdk::Player * target); // it gets owner internally based on weapon
    static void *is_behind_and_facing_target_callgate = signature::find_pattern<is_behind_and_facing_target_fn>("client", "E8 ? ? ? ? 84 C0 74 08 5F B0 01 5E 5D C2 04 00 A1", 0);
    if (!is_behind_and_facing_target_callgate) {
        logging::msg("Can't get IsBehindAndFacingTarget callgate!");
    }
    static is_behind_and_facing_target_fn is_behind_and_facing_target = nullptr;
    if (!is_behind_and_facing_target) {
        is_behind_and_facing_target = (is_behind_and_facing_target_fn)signature::resolve_callgate(is_behind_and_facing_target_callgate);
    }
#else
#error "No is_behind_and_facing_target for Mac OSX! (yet, DIY)"
#endif

    if (!is_behind_and_facing_target) {
        logging::msg("Can't find is_behind_and_facing_target!!!");
        return false;
    }

    auto local_player = Player::local();

    trace::TraceResult trace;
    auto               wep = local_player->active_weapon();
    if (wep->do_swing_trace(&trace)) {
        /*if(trace.fraction==1.0)
            return false;*/
        auto swang = trace.entity->to_player();
        if (swang == nullptr || !swang->is_valid())
            return false;
        if (swang->dormant())
            return false;
        if (local_player->team() == swang->team())
            return false;
        if (!swang->alive())
            return false;
        //logging::msg("%.2f", local_player->origin().distance(swang->origin()));
        return is_behind_and_facing_target(wep, swang); // TODO: add some sort of length check
    }
    return false;
}

void create_move(sdk::UserCmd *cmd) { // TODO: make rage autobackstab
    profiler_profile_function();

    auto local_player = Player::local();
    if (!local_player->is_valid() || local_player->dormant() || !local_player->alive()) // we aren't zombies
        return;

    if (local_player->tf_class() != 8) // Not Spy
        return;

    auto weapon = local_player->active_weapon();
    if (!weapon->is_valid() || weapon->dormant() || weapon->client_class()->class_id != class_id::CTFKnife) // no knife no backstab
        return;                                                                                             // TODO: check for non backstab knifes (if they even exist)

    /*if (legit_check()) {
        cmd->buttons |= 1;
        return; // we found nice target w/o backtrack
    }*/

    backtrack::RewindState rewind;
    auto                   current_tick = iface::globals->tickcount;

    auto delta_delta = 1; // ??? TODO: make customiziable (forward/reverse)
    auto delta       = 1; // ??? ^^^
    do {
        u32 new_tick = current_tick - delta;
        if (backtrack::tick_valid(new_tick)) {
            rewind.to_tick(new_tick);

            if (legit_check()) {   // TODO: do something against wierd swing and missing
                cmd->buttons |= 1; // IN_ATTACK
                cmd->tick_count -= delta;
                break; // we found nice tick and target
            }
        }

        delta += delta_delta;
    } while (delta < backtrack::max_ticks);
}
} // namespace auto_backstab

#include <precompiled.hh>

#include "sdk/sdk.hh"

#include "sdk/hooks.hh"
#include "sdk/log.hh"
#include "sdk/player.hh"

#include "modules/aimbot.hh"
#include "modules/anti_aim.hh"
#include "modules/auto_backstab.hh"
#include "modules/backtrack.hh"
#include "modules/lagexploit.hh"
#include "modules/misc.hh"

#include "utils/math.hh"

#include "utils/profiler.hh"

#if doghook_platform_windows()
#include <intrin.h>
#endif

sdk::UserCmd *last_cmd;

using namespace sdk;

namespace create_move {
bool *send_packet_ptr = nullptr;

bool *send_packet() {
    return send_packet_ptr;
}

// TODO: should probably move elsewhere
static inline auto local_player_prediction(Player *local, UserCmd *cmd) {
    char move_data_buffer[512];
    memset(move_data_buffer, 0, sizeof(move_data_buffer));

    // Setup prediction
    auto old_cur_time   = iface::globals->curtime;
    auto old_frame_time = iface::globals->frametime;
    auto old_tick_count = iface::globals->tickcount;

    iface::globals->curtime = local->tick_base() * iface::globals->interval_per_tick;

    // If we are already not able to fit enough ticks into a frame account for this!!
    iface::globals->frametime = iface::globals->interval_per_tick;
    iface::globals->tickcount = local->tick_base();

    // Set the current usercmd and run prediction
    if constexpr (doghook_platform::windows())
        local->set<UserCmd *, 0x107C>(cmd);
    else if constexpr (doghook_platform::linux())
        local->set<UserCmd *, 0x105C>(cmd);

    iface::prediction->setup_move(local, cmd, iface::move_helper, move_data_buffer);
    iface::game_movement->process_movement(local, move_data_buffer);
    iface::prediction->finish_move(local, cmd, move_data_buffer);

    if constexpr (doghook_platform::windows())
        local->set<UserCmd *, 0x107C>(0);
    else if constexpr (doghook_platform::linux())
        local->set<UserCmd *, 0x105C>(0);

    // Cleanup from prediction
    iface::globals->curtime   = old_cur_time;
    iface::globals->frametime = old_frame_time;
    iface::globals->tickcount = old_tick_count;
}

std::unique_ptr<hooks::HookFunction<ClientMode, 0>> create_move_hook;

#if doghook_platform_windows()
bool __fastcall hooked_create_move(void *instance, void *edx, float sample_framerate, UserCmd *user_cmd)
#else
bool hooked_create_move(void *instance, float sample_framerate, UserCmd *user_cmd)
#endif
{
    profiler_profile_function();

#if doghook_platform_windows()
    uptr      ebp_address;
    __asm mov ebp_address, ebp;
    send_packet_ptr = reinterpret_cast<bool *>(***(uptr ***)ebp_address - 1);
#else
    uptr **fp;
    __asm__("mov %%ebp, %0"
            : "=r"(fp));
    send_packet_ptr = reinterpret_cast<bool *>(**fp - 8);
#endif

    auto local_player = Player::local();
    assert(local_player);

    //bool retval = create_move_hook->call_original<bool>(sample_framerate, user_cmd);
    create_move_hook->call_original<void>(sample_framerate, user_cmd);

    if (user_cmd)
        last_cmd = user_cmd;

    // Do create_move_pre_predict()
    {
        profiler_profile_scope("pre_predict");

        {
            profiler_profile_scope("pre_predict/backtrack");
            backtrack::create_move_pre_predict(user_cmd);
        }
        {
            profiler_profile_scope("pre_predict/aimbot");
            aimbot::create_move_pre_predict(user_cmd);
        }
    }
    {
        profiler_profile_scope("local_player_prediction");
        local_player_prediction(local_player, user_cmd);
    }
    // Do create_move()
    {
        profiler_profile_scope("create_move");

        aimbot::create_move(user_cmd);
        backtrack::create_move(user_cmd);
        //lagexploit::create_move(user_cmd);
        //auto_backstab::create_move(user_cmd);
        anti_aim::create_move(user_cmd);
        misc::create_move(user_cmd);
    }
    {
        profiler_profile_scope("finish");
        backtrack::create_move_finish(user_cmd);
    }

    //return retval;
    return false;
}

void level_init() {
    logging::msg("=> Hooking up!");

    assert(create_move_hook == nullptr);
    create_move_hook = std::make_unique<hooks::HookFunction<ClientMode, 0>>(iface::client_mode, 21, 22, 22, reinterpret_cast<void *>(&hooked_create_move));
}

void level_shutdown() {
    logging::msg("<= Deleting hooks");

    create_move_hook.reset();
    create_move_hook = nullptr;
}

} // namespace create_move

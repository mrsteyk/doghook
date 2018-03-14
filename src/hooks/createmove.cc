#include <precompiled.hh>

#include "sdk/sdk.hh"

#include "sdk/hooks.hh"
#include "sdk/log.hh"
#include "sdk/player.hh"

using namespace sdk;

namespace create_move {
// TODO: should probably move elsewhere
static inline auto local_player_prediction(Player *local, UserCmd *cmd) {
    char move_data_buffer[512];
    memset(move_data_buffer, 0, sizeof(move_data_buffer));

    // Setup prediction
    auto old_cur_time   = IFace<Globals>()->curtime;
    auto old_frame_time = IFace<Globals>()->frametime;
    auto old_tick_count = IFace<Globals>()->tickcount;

    IFace<Globals>()->curtime = local->tick_base() * IFace<Globals>()->interval_per_tick;

    // If we are already not able to fit enough ticks into a frame account for this!!
    IFace<Globals>()->frametime = IFace<Globals>()->interval_per_tick;
    IFace<Globals>()->tickcount = local->tick_base();

    // Set the current usercmd and run prediction
    local->set<UserCmd *, 0x107C>(cmd);

    IFace<Prediction>()->setup_move(local, cmd, IFace<MoveHelper>().get(), move_data_buffer);
    IFace<GameMovement>()->process_movement(local, move_data_buffer);
    IFace<Prediction>()->finish_move(local, cmd, move_data_buffer);

    local->set<UserCmd *, 0x107C>(0);

    // Cleanup from prediction
    IFace<Globals>()->curtime   = old_cur_time;
    IFace<Globals>()->frametime = old_frame_time;
    IFace<Globals>()->tickcount = old_tick_count;
}

hooks::HookFunction<ClientMode, 0> *create_move_hook = nullptr;

#if doghook_platform_windows()
bool __fastcall hooked_create_move(void *instance, void *edx, float sample_framerate, UserCmd *user_cmd)
#else
bool hooked_create_move(void *instance, float sample_framerate, UserCmd *user_cmd)
#endif
{
    auto local_player = Player::local();
    if (local_player == nullptr) return true;

    // TODO: remove
    Log::msg("tick: %d", user_cmd->tick_count);

    // TODO: call original
    // Currently possible but the method is slow
    // Fixme in HookFunction!

    // Do create_move_pre_predict()

    local_player_prediction(local_player, user_cmd);

    // Do create_move()

    return false;
}

void level_init() {
    Log::msg("=> Hooking up!");

    assert(create_move_hook == nullptr);
    create_move_hook = new hooks::HookFunction<ClientMode, 0>(IFace<ClientMode>().get(), 21, reinterpret_cast<void *>(&hooked_create_move));
}

void level_shutdown() {
    Log::msg("<= Deleting hooks");

    delete create_move_hook;
    create_move_hook = nullptr;
}

} // namespace create_move

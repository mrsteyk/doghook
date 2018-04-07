#pragma once

#include <sdk/platform.hh>

namespace sdk {
class UserCmd;
class Player;
class NetChannel;
struct PlayerHitboxes;
} // namespace sdk

namespace backtrack {
// Helper class to rewind all players back to a specific tick
class RewindState {
public:
    RewindState();
    ~RewindState();

    void to_tick(u32 tick);
};

enum { max_ticks = 66 };

void create_move_pre_predict(sdk::UserCmd *cmd);
void create_move(sdk::UserCmd *cmd);
void create_move_finish(sdk::UserCmd *cmd);

float lerp_time();
bool  tick_valid(u32 tick);

u32 hitboxes_for_player(sdk::Player *p, u32 tick, sdk::PlayerHitboxes &hitboxes);

// Returns true if this tick was valid
// You should be using RewindState instead of this...
bool backtrack_player_to_tick(sdk::Player *p, u32 tick, bool set_alive = false, bool restoring = false);

void add_latency_to_netchannel(sdk::NetChannel *channel);
} // namespace backtrack

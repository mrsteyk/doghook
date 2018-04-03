#pragma once

#include <sdk/platform.hh>

namespace sdk {
class UserCmd;
class Player;
struct PlayerHitboxes;
} // namespace sdk

namespace backtrack {
enum { max_ticks = 15 };

void create_move_pre_predict(sdk::UserCmd *cmd);
void create_move(sdk::UserCmd *cmd);
void create_move_finish(sdk::UserCmd *cmd);

// Returns true if this tick was valid
bool backtrack_player_to_tick(sdk::Player *p, u32 tick, bool restoring = false);
bool tick_valid(u32 tick);

void update_player_hitboxes(sdk::Player *p, const sdk::PlayerHitboxes &hitboxes, u32 hitboxes_count);
void hitboxes_for_player(sdk::Player *p, u32 tick, sdk::PlayerHitboxes &hitboxes);
} // namespace backtrack

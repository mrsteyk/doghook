#pragma once

#include "sdk/sdk.hh"

namespace aimbot {
void init_all();

void update(float frametime);

void create_move(sdk::UserCmd *cmd);
void create_move_pre_predict(sdk::UserCmd *cmd);
}; // namespace aimbot

#include "precompiled.hh"

#include "interface.hh"
#include "sdk.hh"

namespace sdk {
i32 Globals::time_to_ticks(float time) {
    return static_cast<i32>((0.5f + (time / iface::globals->interval_per_tick)));
}

float Globals::ticks_to_time(i32 ticks) {
    return ticks * iface::globals->interval_per_tick;
}

namespace iface {
IFace<Client>            client;
IFace<ClientMode>        client_mode;
IFace<Globals>           globals;
IFace<Engine>            engine;
IFace<EntList>           ent_list;
IFace<Input>             input;
IFace<Cvar>              cvar;
IFace<Trace>             trace;
IFace<ModelInfo>         model_info;
IFace<DebugOverlay>      overlay;
IFace<PlayerInfoManager> info_manager;
IFace<MoveHelper>        move_helper;
IFace<Prediction>        prediction;
IFace<GameMovement>      game_movement;
IFace<EngineVgui>        engine_vgui;
IFace<InputSystem>       input_system;
} // namespace iface

} // namespace sdk

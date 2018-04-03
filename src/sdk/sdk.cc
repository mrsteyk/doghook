#include "precompiled.hh"

#include "interface.hh"
#include "sdk.hh"

namespace sdk {
i32 Globals::time_to_ticks(float time) {
    return static_cast<i32>((0.5f + (time / IFace<Globals>()->interval_per_tick)));
}

float Globals::ticks_to_time(i32 ticks) {
    return ticks * IFace<Globals>()->interval_per_tick;
}

} // namespace sdk

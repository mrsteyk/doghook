#pragma once

#include "entity.hh"

#include "trace.hh"

namespace sdk {
class Weapon : public Entity {
public:
    bool can_shoot(u32 tickbase);

    float next_primary_attack();
    float next_secondary_attack();

    u32 clip1();

    Entity *owner();

    bool do_swing_trace(trace::TraceResult* trace);
    int get_swing_range();
};
} // namespace sdk

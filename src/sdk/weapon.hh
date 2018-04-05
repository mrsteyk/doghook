#pragma once

#include "entity.hh"

namespace sdk {
class Weapon : public Entity {
public:
    bool can_shoot(u32 tickbase);

    float next_primary_attack();
    float next_secondary_attack();

    u32 clip1();

    Entity *owner();
};
} // namespace sdk

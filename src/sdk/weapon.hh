#pragma once

#include "entity.hh"

namespace sdk {
class Weapon : public Entity {
public:
    auto can_shoot(u32 tickbase) -> bool;

    auto next_primary_attack() -> float;
    auto next_secondary_attack() -> float;

    auto owner() -> Entity *;
};
} // namespace sdk

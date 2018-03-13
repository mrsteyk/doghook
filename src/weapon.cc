#include "stdafx.hh"

#include "interface.hh"
#include "netvar.hh"
#include "sdk.hh"
#include "weapon.hh"

using namespace sdk;

auto next_primary_attack = Netvar("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextPrimaryAttack");
auto Weapon::next_primary_attack() -> float {
    return ::next_primary_attack.get<float>(this);
}

auto next_secondary_attack = Netvar("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextSecondaryAttack");
auto Weapon::next_secondary_attack() -> float {
    return ::next_secondary_attack.get<float>(this);
}

auto Weapon::can_shoot(u32 tickbase) -> bool {
    return tickbase * IFace<Globals>()->interval_per_tick > next_primary_attack();
}

auto Weapon::owner() -> Entity * {
    // TODO:
    return nullptr;
}

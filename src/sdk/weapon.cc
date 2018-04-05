#include "precompiled.hh"

#include "interface.hh"
#include "netvar.hh"
#include "sdk.hh"
#include "weapon.hh"

using namespace sdk;

auto  next_primary_attack = Netvar("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextPrimaryAttack");
float Weapon::next_primary_attack() {
    return ::next_primary_attack.get<float>(this);
}

auto  next_secondary_attack = Netvar("DT_BaseCombatWeapon", "LocalActiveWeaponData", "m_flNextSecondaryAttack");
float Weapon::next_secondary_attack() {
    return ::next_secondary_attack.get<float>(this);
}

bool Weapon::can_shoot(u32 tickbase) {
    return tickbase * IFace<Globals>()->interval_per_tick > next_primary_attack();
}

auto clip1 = Netvar("DT_BaseCombatWeapon", "LocalWeaponData", "m_iClip1");
u32  Weapon::clip1() {
    return ::clip1.get<u32>(this);
}

Entity *Weapon::owner() {
    // TODO:
    return nullptr;
}

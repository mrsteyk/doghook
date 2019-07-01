#include "precompiled.hh"

#include "interface.hh"
#include "netvar.hh"
#include "sdk.hh"
#include "weapon.hh"

#include "vfunc.hh"

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
    return tickbase * iface::globals->interval_per_tick > next_primary_attack();
}

auto clip1 = Netvar("DT_BaseCombatWeapon", "LocalWeaponData", "m_iClip1");
u32  Weapon::clip1() {
    return ::clip1.get<u32>(this);
}

Entity *Weapon::owner() {
    // TODO:
    return nullptr;
}

bool Weapon::do_swing_trace(trace::TraceResult *trace) {
    return_virtual_func(do_swing_trace, 453, 523, 0, 0, trace); // look for "set_disguise_on_backstab" and look for 2 arg func that has if for '== 1'
}

int Weapon::get_swing_range() {
    return_virtual_func(get_swing_range, 451, 521, 0, 0); // look for "is_a_sword" (3rd XRef rn) and 72, 48, 128 constants
}

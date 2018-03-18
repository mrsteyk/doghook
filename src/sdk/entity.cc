#include "precompiled.hh"

#include "entity.hh"
#include "player.hh"
#include "weapon.hh"

#include "netvar.hh"

#include "sdk.hh"

using namespace sdk;

auto Entity::to_handle() -> EntityHandle & {
    return_virtual_func(to_handle, 2, 3, 3, 0);
}

auto Entity::is_valid() -> bool {
    // get around clangs "correctly formed code never has a null thisptr"
    auto thisptr = reinterpret_cast<uptr>(this);
    if (thisptr == 0) return false;

    return true;
}

auto Entity::to_player() -> class Player * {
    auto clientclass = client_class();

    // TODO: do not hardcode this value
    if (clientclass->class_id == 246) return static_cast<class Player *>(this);

    return nullptr;
}

auto Entity::to_weapon() -> class Weapon * {
    // TODO: checks...
    return static_cast<class Weapon *>(this);
}

auto Entity::client_class() -> struct ClientClass * {
    return_virtual_func(client_class, 2, 17, 17, 8);
}

auto Entity::dormant() -> bool {
    return_virtual_func(dormant, 8, 75, 75, 8);
}

auto Entity::index() -> u32 {
    return_virtual_func(index, 9, 79, 79, 8);
}

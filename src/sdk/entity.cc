#include "precompiled.hh"

#include "entity.hh"
#include "player.hh"
#include "weapon.hh"

#include "netvar.hh"

#include "class_id.hh"

#include "sdk.hh"

using namespace sdk;

EntityHandle &Entity::to_handle() {
    return_virtual_func(to_handle, 2, 3, 3, 0);
}

bool Entity::is_valid() {
    // get around clangs "correctly formed code never has a null thisptr"
    auto thisptr = reinterpret_cast<uptr>(this);
    if (thisptr == 0) return false;

    return true;
}

class Player *Entity::to_player() {
    auto clientclass = client_class();

    if (clientclass->class_id == sdk::class_id::CTFPlayer) return static_cast<class Player *>(this);

    return nullptr;
}

class Weapon *Entity::to_weapon() {
    // TODO: checks...
    return static_cast<class Weapon *>(this);
}

struct ClientClass *Entity::client_class() {
    return_virtual_func(client_class, 2, 17, 17, 8);
}

bool Entity::dormant() {
    return_virtual_func(dormant, 8, 75, 75, 8);
}

u32 Entity::index() {
    return_virtual_func(index, 9, 79, 79, 8);
}

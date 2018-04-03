#include <precompiled.hh>

#include "trace.hh"

#include "sdk.hh"

bool trace::Filter::should_hit_entity(sdk::Entity *handle_entity, int contents_mask) {
    auto handle      = handle_entity->to_handle();
    auto real_entity = IFace<sdk::EntList>()->from_handle(handle);

    if (real_entity == nullptr) return false;

    auto client_class = real_entity->client_class();

    // ignore "bad" entities
    switch (client_class->class_id) {
    case 64:  // CFuncRespawnRoomVisualizer
    case 230: // CTFMedigunShield
    case 55:  // CFuncAreaPortalWindow
        return false;
    }

    if (real_entity == ignore_self ||
        real_entity == ignore_entity) return false;

    return true;
}

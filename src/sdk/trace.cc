#include <precompiled.hh>

#include "trace.hh"

#include "sdk.hh"

#include "class_id.hh"

bool trace::Filter::should_hit_entity(sdk::Entity *handle_entity, int contents_mask) {
    auto handle      = handle_entity->to_handle();
    auto real_entity = sdk::iface::ent_list->from_handle(handle);

    if (real_entity == nullptr) return false;

    auto client_class = real_entity->client_class();

    // ignore "bad" entities
    switch (client_class->class_id) {
    case sdk::class_id::CFuncRespawnRoomVisualizer:
    case sdk::class_id::CTFMedigunShield:
    case sdk::class_id::CFuncAreaPortalWindow:
        return false;
    }

    if (real_entity == ignore_self ||
        real_entity == ignore_entity) return false;

    return true;
}

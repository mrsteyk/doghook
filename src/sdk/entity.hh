#pragma once

#include "types.hh"
#include "vfunc.hh"

namespace sdk {
struct EntityHandle {
    u32 serial_index;
};

class Entity {
public:
    Entity() = delete;

    // helper functions
    bool is_valid();

    EntityHandle &to_handle();

    template <typename T, u32 offset>
    auto set(T data) { *reinterpret_cast<T *>(reinterpret_cast<uptr>(this) + offset) = data; }

    template <typename T>
    auto set(u32 offset, T data) { *reinterpret_cast<T *>(reinterpret_cast<uptr>(this) + offset) = data; }

    template <typename T, u32 offset>
    auto &get() { return *reinterpret_cast<T *>(reinterpret_cast<uptr>(this) + offset); }

    template <typename T>
    auto &get(u32 offset) { return *reinterpret_cast<T *>(reinterpret_cast<uptr>(this) + offset); }

    // upcasts
    class Player *to_player();
    class Weapon *to_weapon();

    // virtual functions
    struct ClientClass *client_class();

    bool dormant();
    u32  index();
};
} // namespace sdk

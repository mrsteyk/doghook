#pragma once

#include "types.hh"

#include "entity.hh"

#undef min
#undef max

namespace sdk {
struct PlayerHitboxes {
    math::Vector centre[128];

    math::Vector min[128];
    math::Vector max[128];

#ifdef _DEBUG
    // These are used for debugging
    math::Vector origin[128];
    math::Vector rotation[128];

    math::Vector raw_min[128];
    math::Vector raw_max[128];
#endif
};

class Player : public Entity {
public:
    Player() = delete;

    // helper functions
    static auto local() -> Player *;

    auto world_space_centre() -> math::Vector &;

    auto model_handle() -> const class ModelHandle *;
    auto studio_model() -> const class StudioModel *;

    auto view_position() -> math::Vector;

    auto hitboxes(PlayerHitboxes *hitboxes_out, bool create_pose) -> u32;

    // netvars
    auto health() -> int &;

    auto alive() -> bool;
    auto team() -> int;

    auto cond() -> u32 &;

    auto view_offset() -> math::Vector &;

    auto tf_class() -> int;

    auto tick_base() -> int &;
    auto sim_time() -> float &;
    auto anim_time() -> float &;
    auto cycle() -> float &;
    auto sequence() -> int &;

    auto fov_time() -> float;

    auto render_origin() -> math::Vector &;

    auto active_weapon() -> Weapon *;

    // virtual functions
    auto origin() -> math::Vector &;
    auto set_origin(const math::Vector &v) -> void;

    auto render_bounds() -> std::pair<math::Vector, math::Vector>;

    auto angles() -> math::Vector &;
    auto set_angles(const math::Vector &v) -> void;

    auto anim_layer(u32 index) -> class AnimationLayer &;
    auto anim_layer_count() -> u32;

    auto update_client_side_animation() -> void;

    auto invalidate_physics_recursive(u32 flags) -> void;

    // These are relative to the origin
    auto collision_bounds() -> std::pair<math::Vector &, math::Vector &>;

    auto bone_transforms(math::Matrix3x4 *hitboxes_out, u32 max_bones, u32 bone_mask, float current_time) -> bool;
};
} // namespace sdk

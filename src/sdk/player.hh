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

    // These are NOT in bone order - they are in the order that
    // bone_transforms() returns so you will need to index by box->bone
    math::Matrix3x4 bone_to_world[128];

#ifdef _DEBUG
    // These are used for debugging
    math::Vector origin[128];
    math::Vector rotation[128];

    math::Vector raw_min[128];
    math::Vector raw_max[128];
#endif
};

struct PlayerInfo {
    char name[32];
    int  user_id;
    char guid[33];
    u32  friends_id;
    char friends_name[32];
    bool fake_player;
    bool is_hltv;
    u32  custom_files[4];
    u32  files_downloaded;
};

class Player : public Entity {
public:
    Player() = delete;

    // helper functions
    static Player *local();

    math::Vector world_space_centre();

    const class ModelHandle *model_handle();
    const class StudioModel *studio_model();

    math::Vector view_position();

    u32 hitboxes(PlayerHitboxes *hitboxes_out, bool create_pose);

    // If the player is not found user_id will be -1
    PlayerInfo info();

    // netvars
    int &health();

    bool alive();
    int  team();

    u32 &cond();

    math::Vector &view_offset();

    int tf_class();

    int &  tick_base();
    float &sim_time();
    float &anim_time();
    float &cycle();
    int &  sequence();

    float fov_time();

    math::Vector &render_origin();
    math::Vector &render_angle();

    Weapon *active_weapon();

    // virtual functions
    math::Vector &origin();
    void          set_origin(const math::Vector &v);

    std::pair<math::Vector, math::Vector> render_bounds();

    math::Vector &angles();
    void          set_angles(const math::Vector &v);

    class AnimationLayer &anim_layer(u32 index);
    u32                   anim_layer_count();

    void update_client_side_animation();

    void invalidate_physics_recursive(u32 flags);

    // These are relative to the origin
    std::pair<math::Vector &, math::Vector &> collision_bounds();

    bool bone_transforms(math::Matrix3x4 *hitboxes_out, u32 max_bones, u32 bone_mask, float current_time);
};
} // namespace sdk

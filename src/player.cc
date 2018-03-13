#include "stdafx.hh"

#include "player.hh"
#include "weapon.hh"

#include "netvar.hh"
#include "vfunc.hh"

#include "sdk.hh"

using namespace sdk;

auto Player::local() -> Player * {
    return static_cast<Player *>(IFace<EntList>()->entity(IFace<Engine>()->local_player_index()));
}

static auto health = Netvar("DT_BasePlayer", "m_iHealth");
auto        Player::health() -> int & {
    return ::health.get<int>(this);
}

static auto lifestate = Netvar("DT_BasePlayer", "m_lifeState");
auto        Player::alive() -> bool {
    return ::lifestate.get<u8>(this) == 0;
}

auto Player::origin() -> math::Vector & {
    return_virtual_func(origin, 9, 0, 0, 0);
}

auto Player::set_origin(const math::Vector &v) -> void {
    if constexpr (doghook_platform::windows()) {
        static auto original = signature::find_pattern<void(__thiscall *)(Player *, const math::Vector &)>("client", "55 8B EC 56 57 8B F1 E8 ? ? ? ? 8B 7D 08 F3 0F 10 07", 0);
        assert(original);
        return original(this, v);
    } else if constexpr (doghook_platform::linux()) {
        static_assert(doghook_platform::linux() == false);
    } else if constexpr (doghook_platform::osx()) {
        static_assert(doghook_platform::osx() == false);
    }
}

auto Player::angles() -> math::Vector & {
    return_virtual_func(angles, 10, 0, 0, 0);
}

auto Player::set_angles(const math::Vector &v) -> void {
    if constexpr (doghook_platform::windows()) {
        static auto original = signature::find_pattern<void(__thiscall *)(Player *, const math::Vector &)>("client", "55 8B EC 83 EC 60 56 57 8B F1", 0);
        assert(original);
        return original(this, v);
    } else if constexpr (doghook_platform::linux()) {
        static_assert(doghook_platform::linux() == false);
    } else if constexpr (doghook_platform::osx()) {
        static_assert(doghook_platform::osx() == false);
    }
}

static auto team = Netvar("DT_BaseEntity", "m_iTeamNum");
auto        Player::team() -> int {
    return ::team.get<int>(this);
}

static auto cond = Netvar("DT_TFPlayer", "m_Shared", "m_nPlayerCond");
auto        Player::cond() -> u32 & {
    return ::cond.get<u32>(this);
}

auto Player::render_bounds() -> std::pair<math::Vector, math::Vector> {
    auto func = vfunc::Func<void (Player::*)(math::Vector &, math::Vector &)>(this, 20, 0, 0, 4);

    std::pair<math::Vector, math::Vector> ret;

    func.invoke(ret.first, ret.second);

    auto origin = this->origin();

    ret.first += origin;
    ret.second += origin;

    return ret;
}

static auto collideable_min = Netvar("DT_BaseEntity", "m_Collision", "m_vecMinsPreScaled");
static auto collideable_max = Netvar("DT_BaseEntity", "m_Collision", "m_vecMaxsPreScaled");
auto        Player::collision_bounds() -> std::pair<math::Vector &, math::Vector &> {

    auto &min = ::collideable_min.get<math::Vector>(this);
    auto &max = ::collideable_max.get<math::Vector>(this);

    return std::make_pair(std::ref(min), std::ref(max));
}

static auto view_offset = Netvar("DT_BasePlayer", "localdata", "m_vecViewOffset[0]");
auto        Player::view_offset() -> math::Vector & {
    return ::view_offset.get<math::Vector>(this);
}

static auto tf_class = Netvar("DT_TFPlayer", "m_PlayerClass", "m_iClass");
auto        Player::tf_class() -> int {
    return ::tf_class.get<int>(this);
}

static auto tick_base = Netvar("DT_BasePlayer", "localdata", "m_nTickBase");
auto        Player::tick_base() -> int {
    return ::tick_base.get<int>(this);
}

static auto active_weapon_handle = Netvar("DT_BaseCombatCharacter", "m_hActiveWeapon");
auto        Player::active_weapon() -> Weapon * {
    return static_cast<Weapon *>(IFace<EntList>()->from_handle(::active_weapon_handle.get<EntityHandle>(this)));
}

static auto sim_time = Netvar("DT_BaseEntity", "m_flSimulationTime");
auto        Player::sim_time() -> float & {
    return ::sim_time.get<float>(this);
}

static auto anim_time = Netvar("DT_BaseEntity", "AnimTimeMustBeFirst", "m_flAnimTime");
auto        Player::anim_time() -> float & {
    return ::anim_time.get<float>(this);
}

static auto cycle = Netvar("DT_BaseAnimating", "serveranimdata", "m_flCycle");
auto        Player::cycle() -> float & {
    return ::cycle.get<float>(this);
}

static auto fov_time = Netvar("DT_BasePlayer", "m_flFOVTime");
auto        Player::fov_time() -> float {
    return ::fov_time.get<float>(this);
}

static auto render_origin = Netvar("DT_BaseEntity", "m_vecOrigin");
auto        Player::render_origin() -> math::Vector & {
    return ::render_origin.get<math::Vector>(this);
}

template <typename T>
struct UtlVector {
    T * mem;
    int alloc_count;
    int grow_size;
    int size;
    T * dbg_elements;
};

static auto player_anim_layer_vector(Player *p) -> UtlVector<AnimationLayer> & {
    // TODO: we need a better, cross platform method for doing this.
    // check for "%8.4f : %30s : %5.3f : %4.2f : %1d\n"

    if constexpr (doghook_platform::windows()) {
        UtlVector<AnimationLayer> *anim_overlays = reinterpret_cast<UtlVector<AnimationLayer> *>(p + 2216);
        return *anim_overlays;
    } else if constexpr (doghook_platform::linux()) {
    } else if constexpr (doghook_platform::osx()) {
    }
}

auto Player::anim_layer(u32 index) -> AnimationLayer & {
    return player_anim_layer_vector(this).mem[index];
}

auto Player::anim_layer_count() -> u32 {
    return player_anim_layer_vector(this).size;
}

auto Player::update_client_side_animation() -> void {
    // Look for the string "UpdateClientSideAnimations
    return_virtual_func(update_client_side_animation, 191, 0, 0, 0);
}

auto Player::invalidate_physics_recursive(u32 flags) -> void {
    typedef void(__thiscall * InvalidatePhysicsRecursiveFn)(Player *, u32 flags);

    if constexpr (doghook_platform::windows()) {
        static auto fn = signature::find_pattern<InvalidatePhysicsRecursiveFn>("client", "55 8B EC 51 53 8B 5D 08 56 8B F3 83 E6 04", 0);
        fn(this, flags);
    } else if constexpr (doghook_platform::linux()) {
    } else if constexpr (doghook_platform::osx()) {
    }
}

static auto sequence = Netvar("DT_BaseAnimating", "m_nSequence");
auto        Player::sequence() -> int & {
    return ::sequence.get<int>(this);
}

auto Player::view_position() -> math::Vector {
    return origin() + view_offset();
}

auto Player::bone_transforms(math::Matrix3x4 *hitboxes_out, u32 max_bones, u32 bone_mask, float current_time) -> bool {
    return_virtual_func(bone_transforms, 16, 0, 0, 4, hitboxes_out, max_bones, bone_mask, current_time);
}

auto Player::model_handle() -> const ModelHandle * {
    return_virtual_func(model_handle, 9, 0, 0, 4);
}

auto Player::studio_model() -> const StudioModel * {
    return IFace<ModelInfo>()->studio_model(this->model_handle());
}

static auto get_hitboxes_internal(Player *player, const StudioModel *model, PlayerHitboxes *hitboxes, bool create_pose) {
    math::Matrix3x4 bone_to_world[128];

    // #define BONE_USED_BY_ANYTHING        0x0007FF00
    bool success = player->bone_transforms(bone_to_world, 128, 0x0007FF00, IFace<Engine>()->last_timestamp());
    assert(success);

    auto hitbox_set_ptr = model->hitbox_set(0);
    assert(hitbox_set_ptr);

    auto &hitbox_set = *hitbox_set_ptr;

    auto hitboxes_count = std::min(128u, hitbox_set.hitboxes_count);

    math::Vector origin;
    math::Vector centre;

    for (u32 i = 0; i < hitboxes_count; ++i) {
        auto box = hitbox_set[i];
        assert(box);

        math::Vector rotation;
        math::matrix_angles(bone_to_world[box->bone], rotation, origin);

        math::Matrix3x4 rotate_matrix;
        rotate_matrix.from_angle(rotation);

        math::Vector rotated_min, rotated_max;

        rotated_min = rotate_matrix.rotate_vector(box->min);
        rotated_max = rotate_matrix.rotate_vector(box->max);

        centre = rotated_min.lerp(rotated_max, 0.5);

        hitboxes->centre[i] = origin + centre;
        hitboxes->min[i]    = origin + rotated_min;
        hitboxes->max[i]    = origin + rotated_max;

#ifdef _DEBUG
        hitboxes->rotation[i] = rotation;
        hitboxes->origin[i]   = origin;
        hitboxes->raw_min[i]  = box->min;
        hitboxes->raw_max[i]  = box->max;
#endif
    }

    return hitboxes_count;
}

auto Player::hitboxes(PlayerHitboxes *hitboxes_out, bool create_pose) -> u32 {
    auto model = studio_model();
    assert(model);

    return get_hitboxes_internal(this, model, hitboxes_out, create_pose);
}

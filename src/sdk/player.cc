#include "precompiled.hh"

#include "player.hh"
#include "weapon.hh"

#include "netvar.hh"
#include "vfunc.hh"

#include "log.hh"
#include "sdk.hh"

using namespace sdk;

Player *Player::local() {
    return static_cast<Player *>(IFace<EntList>()->entity(IFace<Engine>()->local_player_index()));
}

static auto health = Netvar("DT_BasePlayer", "m_iHealth");
int &       Player::health() {
    return ::health.get<int>(this);
}

static auto lifestate = Netvar("DT_BasePlayer", "m_lifeState");
u8 &        Player::life_state() {
    return ::lifestate.get<u8>(this);
}

bool Player::alive() {
    return ::lifestate.get<u8>(this) == 0;
}

math::Vector &Player::origin() {
    return_virtual_func(origin, 9, 11, 11, 0);
}

void Player::set_origin(const math::Vector &v) {
    // Look for "Ignoring unreasonable position (%f,%f,%f) from vphysics! (entity %s)\n"

    if constexpr (doghook_platform::windows()) {
        static auto original = signature::find_pattern<void(__thiscall *)(Player *, const math::Vector &)>("client", "55 8B EC 56 57 8B F1 E8 ? ? ? ? 8B 7D 08 F3 0F 10 07", 0);
        return original(this, v);
    } else if constexpr (doghook_platform::linux()) {
        static auto original = signature::find_pattern<void(__thiscall *)(Player *, const math::Vector &)>("client", "55 89 E5 57 56 53 83 EC 1C 8B 5D 08 8B 75 0C 89 1C 24 E8 ? ? ? ? F3 0F 10 06", 0);
        return original(this, v);
    } else if constexpr (doghook_platform::osx()) {
    }
}

math::Vector &Player::angles() {
    return_virtual_func(angles, 10, 12, 12, 0);
}

void Player::set_angles(const math::Vector &v) {
    // Look for "Ignoring bogus angles (%f,%f,%f) from vphysics! (entity %s)\n"

    if constexpr (doghook_platform::windows()) {
        static auto original = signature::find_pattern<void(__thiscall *)(Player *, const math::Vector &)>("client", "55 8B EC 83 EC 60 56 57 8B F1", 0);
        assert(original);
        return original(this, v);
    } else if constexpr (doghook_platform::linux()) {
        static auto original = signature::find_pattern<void(__thiscall *)(Player *, const math::Vector &)>("client", "55 89 E5 57 56 53 81 EC 8C 00 00 00 8B 5D 08 8B 75 0C 89 1C 24", 0);
        return original(this, v);
    } else if constexpr (doghook_platform::osx()) {
    }
}

static auto team = Netvar("DT_BaseEntity", "m_iTeamNum");
int         Player::team() {
    return ::team.get<int>(this);
}

static auto cond = Netvar("DT_TFPlayer", "m_Shared", "m_nPlayerCond");
u32 &       Player::cond() {
    return ::cond.get<u32>(this);
}

std::pair<math::Vector, math::Vector> Player::render_bounds() {
    auto func = vfunc::Func<void (Player::*)(math::Vector &, math::Vector &)>(this, 20, 60, 60, 4);

    std::pair<math::Vector, math::Vector> ret;

    func.invoke(ret.first, ret.second);

    return ret;
}

math::Vector Player::world_space_centre() {
    auto bounds = render_bounds();
    auto centre = origin();

    centre.z += (bounds.first.z + bounds.second.z) / 2;
    return centre;
}

static auto                               collideable_min = Netvar("DT_BaseEntity", "m_Collision", "m_vecMinsPreScaled");
static auto                               collideable_max = Netvar("DT_BaseEntity", "m_Collision", "m_vecMaxsPreScaled");
std::pair<math::Vector &, math::Vector &> Player::collision_bounds() {

    auto &min = ::collideable_min.get<math::Vector>(this);
    auto &max = ::collideable_max.get<math::Vector>(this);

    return std::make_pair(std::ref(min), std::ref(max));
}

static auto   view_offset = Netvar("DT_BasePlayer", "localdata", "m_vecViewOffset[0]");
math::Vector &Player::view_offset() {
    return ::view_offset.get<math::Vector>(this);
}

static auto tf_class = Netvar("DT_TFPlayer", "m_PlayerClass", "m_iClass");
int         Player::tf_class() {
    return ::tf_class.get<int>(this);
}

static auto tick_base = Netvar("DT_BasePlayer", "localdata", "m_nTickBase");
int &       Player::tick_base() {
    return ::tick_base.get<int>(this);
}

static auto active_weapon_handle = Netvar("DT_BaseCombatCharacter", "m_hActiveWeapon");
Weapon *    Player::active_weapon() {
    return static_cast<Weapon *>(IFace<EntList>()->from_handle(::active_weapon_handle.get<EntityHandle>(this)));
}

static auto sim_time = Netvar("DT_BaseEntity", "m_flSimulationTime");
float &     Player::sim_time() {
    return ::sim_time.get<float>(this);
}

static auto anim_time = Netvar("DT_BaseEntity", "AnimTimeMustBeFirst", "m_flAnimTime");
float &     Player::anim_time() {
    return ::anim_time.get<float>(this);
}

static auto cycle = Netvar("DT_BaseAnimating", "serveranimdata", "m_flCycle");
float &     Player::cycle() {
    return ::cycle.get<float>(this);
}

static auto fov_time = Netvar("DT_BasePlayer", "m_flFOVTime");
float       Player::fov_time() {
    return ::fov_time.get<float>(this);
}

static auto   render_origin = Netvar("DT_BaseEntity", "m_vecOrigin");
math::Vector &Player::render_origin() {
    return ::render_origin.get<math::Vector>(this);
}
static auto   render_angle = Netvar("DT_BaseEntity", "m_angRotation");
math::Vector &Player::render_angle() {
    return ::render_angle.get<math::Vector>(this);
}

template <typename T>
struct UtlVector {
    T * mem;
    int alloc_count;
    int grow_size;
    int size;
    T * dbg_elements;
};

static UtlVector<AnimationLayer> &player_anim_layer_vector(Player *p) {
    // TODO: we need a better, cross platform method for doing this.
    // check for "%8.4f : %30s : %5.3f : %4.2f : %1d\n"

    if constexpr (doghook_platform::windows()) {
        UtlVector<AnimationLayer> *anim_overlays = reinterpret_cast<UtlVector<AnimationLayer> *>(p + 2216);
        return *anim_overlays;
    } else if constexpr (doghook_platform::linux()) {
        UtlVector<AnimationLayer> *anim_overlays = reinterpret_cast<UtlVector<AnimationLayer> *>(p + 2196);
        return *anim_overlays;
    } else if constexpr (doghook_platform::osx()) {
        assert(0);
    }
}

AnimationLayer &Player::anim_layer(u32 index) {
    return player_anim_layer_vector(this).mem[index];
}

u32 Player::anim_layer_count() {
    return player_anim_layer_vector(this).size;
}

void Player::update_client_side_animation() {
    // Look for the string "UpdateClientSideAnimations
    return_virtual_func(update_client_side_animation, 191, 253, 253, 0);
}

void Player::invalidate_physics_recursive(u32 flags) {
    // Look for "(%d): Cycle latch used to correct %.2f in to %.2f instead of %.2f"

    typedef void(__thiscall * InvalidatePhysicsRecursiveFn)(Player *, u32 flags);

    if constexpr (doghook_platform::windows()) {
        static auto fn = signature::find_pattern<InvalidatePhysicsRecursiveFn>("client", "55 8B EC 51 53 8B 5D 08 56 8B F3 83 E6 04", 0);
        return fn(this, flags);
    } else if constexpr (doghook_platform::linux()) {
        static auto fn = signature::find_pattern<InvalidatePhysicsRecursiveFn>("client", "55 89 E5 57 56 53 83 EC 1C 8B 75 0C 8B 5D 08 89 F7", 0);
        return fn(this, flags);
    } else if constexpr (doghook_platform::osx()) {
    }
}

static auto sequence = Netvar("DT_BaseAnimating", "m_nSequence");
int &       Player::sequence() {
    return ::sequence.get<int>(this);
}

math::Vector Player::view_position() {
    return origin() + view_offset();
}

bool Player::bone_transforms(math::Matrix3x4 *hitboxes_out, u32 max_bones, u32 bone_mask, float current_time) {
    return_virtual_func(bone_transforms, 16, 96, 96, 4, hitboxes_out, max_bones, bone_mask, current_time);
}

const ModelHandle *Player::model_handle() {
    return_virtual_func(model_handle, 9, 55, 55, 4);
}

const StudioModel *Player::studio_model() {
    return IFace<ModelInfo>()->studio_model(this->model_handle());
}

static auto hitboxes_internal(Player *player, const StudioModel *model, PlayerHitboxes *hitboxes, bool create_pose) {
    math::Matrix3x4 bone_to_world[128];

    // #define BONE_USED_BY_ANYTHING        0x0007FF00
    // #define BONE_USED_BY_HITBOX			0x00000100
    bool success = player->bone_transforms(bone_to_world, 128, 0x00000100, IFace<Globals>()->curtime);
    assert(success);

    std::memcpy(hitboxes->bone_to_world, bone_to_world, 128 * sizeof(math::Matrix3x4));

    auto hitbox_set_ptr = model->hitbox_set(0);
    assert(hitbox_set_ptr);

    // Allow us to use operator[] properly
    auto &hitbox_set = *hitbox_set_ptr;

    auto hitboxes_count = std::min(128u, hitbox_set.hitboxes_count);

    math::Vector centre;

    for (u32 i = 0; i < hitboxes_count; ++i) {
        auto box = hitbox_set[i];
        assert(box);

        const auto &transform = bone_to_world[box->bone];

        auto rotated_min = transform.vector_transform(box->min);
        auto rotated_max = transform.vector_transform(box->max);

        centre = rotated_min.lerp(rotated_max, 0.5);

        hitboxes->centre[i] = centre;
        hitboxes->min[i]    = rotated_min;
        hitboxes->max[i]    = rotated_max;

#ifdef _DEBUG
        math::Vector rotation;
        math::Vector origin;
        math::matrix_angles(transform, rotation, origin);

        hitboxes->rotation[i] = rotation;
        hitboxes->origin[i]   = origin;
        hitboxes->raw_min[i]  = box->min;
        hitboxes->raw_max[i]  = box->max;
#endif
    }

    return hitboxes_count;
}

u32 Player::hitboxes(PlayerHitboxes *hitboxes_out, bool create_pose) {
    auto model = studio_model();
    assert(model);

    return hitboxes_internal(this, model, hitboxes_out, create_pose);
}

PlayerInfo Player::info() {
    PlayerInfo info;
    auto       found = IFace<Engine>()->player_info(index(), &info);

    if (!found) {
        info.user_id = -1;
    }
    return info;
}

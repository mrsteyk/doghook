#include <precompiled.hh>

#include "aimbot.hh"

// TODO:
#include "backtrack.hh"

#include "sdk/class_id.hh"
#include "sdk/entity.hh"
#include "sdk/interface.hh"
#include "sdk/player.hh"
#include "sdk/sdk.hh"
#include "sdk/weapon.hh"

#include "sdk/convar.hh"

#include "sdk/log.hh"
#include "utils/math.hh"
#include "utils/profiler.hh"

#include <algorithm>

using namespace sdk;

namespace aimbot {
using Target = std::pair<Entity *, math::Vector>;

std::vector<Target> targets;

bool can_find_targets = false;

Player *local_player;
Weapon *local_weapon;

int          local_team;
math::Vector local_view;

// TODO: HORRIBLE HACK
// this needs to be done on a per target basis and not like this !!!
u32 cmd_delta = 0;

// Check it a point is visible to the player
static auto visible_no_entity(const math::Vector &position) {
    trace::TraceResult result;
    trace::Ray         ray;
    trace::Filter      f(local_player);

    ray.init(local_view, position);

    IFace<Trace>()->trace_ray(ray, 0x46004003, &f, &result);

    return result.fraction == 1.0f;
}

static Convar<bool> doghook_aimbot_pedantic_mode{"doghook_aimbot_pedantic_mode", true, nullptr};
static auto         visible(Entity *e, const math::Vector &position, const int hitbox) {
    profiler_profile_function();

    trace::TraceResult result;
    trace::Ray         ray;
    trace::Filter      f(local_player);

    ray.init(local_view, position);

    IFace<Trace>()->trace_ray(ray, 0x46004003, &f, &result);

    if (doghook_aimbot_pedantic_mode == true) {
        if (result.entity == e && result.hitbox == hitbox) return true;
    } else if (result.entity == e) {
        return true;
    }

    return false;
}

static auto multipoint_internal(Entity *e, float granularity, const int hitbox, const math::Vector &centre,
                                const math::Vector &min, const math::Vector &max, math::Vector &out) {
    profiler_profile_function();

    // go from centre to centre min first
    for (float i = 0.0f; i <= 1.0f; i += granularity) {
        math::Vector point = centre.lerp(min, i);

        if (visible(e, point, hitbox)) {
            out = point;
            return true;
        }
    }

    // now from centre to max
    for (float i = 0.0f; i <= 1.0f; i += granularity) {
        math::Vector point = centre.lerp(max, i);

        if (visible(e, point, hitbox)) {
            out = point;
            return true;
        }
    }

    return false;
}

// TODO: remove
static Convar<float> doghook_aimbot_multipoint_granularity{"doghook_aimbot_multipoint_granularity", 0, 0, 10, nullptr};

// TODO: there must be some kind of better conversion we can use here to get a straight line across the hitbox
static auto multipoint(Player *player, const int hitbox, const math::Vector &centre, const math::Vector &min, const math::Vector &max, math::Vector &position_out) {
    profiler_profile_function();

    // create a divisor out of the granularity
    float divisor = doghook_aimbot_multipoint_granularity;
    if (divisor == 0) return false;
    float granularity = 1.0f / divisor;

    auto new_x = math::lerp(0.5, min.x, max.x);

    // Create a horizontal cross shape out of this box instead of top left bottom right or visa versa
    math::Vector centre_min_x = math::Vector(new_x, min.y, centre.z);
    math::Vector centre_max_x = math::Vector(new_x, max.y, centre.z);

    if (multipoint_internal(player, granularity, hitbox, centre, centre_min_x, centre_max_x, position_out))
        return true;

    math::Vector centre_min_y = math::Vector(min.x, math::lerp(0.5, min.y, max.y), centre.z);
    math::Vector centre_max_y = math::Vector(max.x, math::lerp(0.5, min.y, max.y), centre.z);

    if (multipoint_internal(player, granularity, hitbox, centre, centre_min_y, centre_max_y, position_out))
        return true;

    return false;
}

static auto find_best_box() {
    auto tf_class        = local_player->tf_class();
    auto weapon_class_id = local_weapon->client_class()->class_id;

    switch (tf_class) {
    case 2:                                                                              // sniper
        if (weapon_class_id == class_id::CTFSniperRifle) return std::make_pair(0, true); // aim head with the rifle
    default:
        return std::make_pair(3, false); // chest
    }
}

static Convar<bool> doghook_aimbot_enable_backtrack{"doghook_aimbot_enable_backtrack", true, nullptr};
static Convar<bool> doghook_aimbot_reverse_backtrack_order{"doghook_aimbot_reverse_backtrack_order", true, nullptr};

auto visible_target_inner(Player *player, std::pair<int, bool> best_box, u32 current_tick, u32 delta, PlayerHitboxes &hitboxes, u32 hitboxes_count, math::Vector &pos) {
    if (delta > 0) {
        auto success = backtrack::backtrack_player_to_tick(player, current_tick - delta);
        if (success == false) return false;

        backtrack::hitboxes_for_player(player, current_tick - delta, hitboxes);
    }
    // check best hitbox first
    if (visible(player, hitboxes.centre[best_box.first], best_box.first)) {
        pos = hitboxes.centre[best_box.first];
        return true;
    } else if (multipoint(player, best_box.first, hitboxes.centre[best_box.first], hitboxes.min[best_box.first], hitboxes.max[best_box.first], pos)) {
        return true;
    }

    // .second is whether we should only check the best box
    if (!best_box.second) {
        for (u32 i = 0; i < hitboxes_count; i++) {
            if (visible(player, hitboxes.centre[i], i)) {
                pos = hitboxes.centre[i];
                return true;
            }
        }

#if 0
        // Perform multiboxing after confirming that we do not have any other options
        for (u32 i = 0; i < hitboxes_count; i++) {
            if (multipoint(player, i, hitboxes.centre[i], hitboxes.min[i], hitboxes.max[i], pos)) {
                return true;
            }
        }
#endif
    }

    return false;
}

auto visible_target(Entity *e, math::Vector &pos) {
    profiler_profile_function();

    // TODO: should entity have a to_player_nocheck() method
    // as we already know at this point that this is a player...
    auto player = e->to_player();

    PlayerHitboxes hitboxes;
    auto           hitboxes_count = player->hitboxes(&hitboxes, false);

    // Tell backtrack about these hitboxes
    backtrack::update_player_hitboxes(player, hitboxes, hitboxes_count);

    auto current_tick  = IFace<Globals>()->tickcount;
    auto best_box      = find_best_box();
    auto reverse_order = !!doghook_aimbot_reverse_backtrack_order;

    if (!reverse_order) {
        // Do no backtrack first
        auto visible = visible_target_inner(player, best_box, current_tick, 0, hitboxes, hitboxes_count, pos);
        if (visible) return true;
    }

    if (!doghook_aimbot_enable_backtrack) return false;

    // If we are going in reverse order then make sure that happens
    const auto delta_delta = doghook_aimbot_reverse_backtrack_order ? -1 : 1;
    auto       delta       = doghook_aimbot_reverse_backtrack_order ? backtrack::max_ticks : 0;

    u32 new_tick;

    do {
        // Go onto the next tick and see what
        delta += delta_delta;
        new_tick = current_tick - delta;
        if (!backtrack::tick_valid(new_tick)) continue;

        if (backtrack::backtrack_player_to_tick(player, current_tick - delta)) {
            auto visible = visible_target_inner(player, best_box, current_tick, delta, hitboxes, hitboxes_count, pos);
            if (visible) {
                cmd_delta = delta;
                return true;
            }
        }

    } while (delta > 0 && delta < backtrack::max_ticks);

    return false;
}

auto valid_target(Entity *e) {
    if (auto player = e->to_player()) {
        if (!player->alive()) return false;
        if (local_team == player->team()) return false;

        return true;
    }

    return false;
}

void finished_target(Target t) {
    assert(t.first != nullptr);

    IFace<DebugOverlay>()->add_entity_text_overlay(t.first->index(), 2, 0, 255, 255, 255, 255, "finished");

    targets.push_back(t);
}

auto sort_targets() {
    profiler_profile_function();

    std::sort(targets.begin(), targets.end(),
              [](const Target &a, const Target &b) {
                  // Ignore null targets (artifact of having a resized vector mess
                  if (a.first == nullptr) return false;
                  if (b.first == nullptr) return false;

                  return a.second.distance(local_view) < b.second.distance(local_view);
              });
}

auto find_targets() {
    profiler_profile_function();

    if (can_find_targets == false) return;

    // find targets
    // TODO: clean up this mess
    for (auto e : IFace<EntList>()->get_range()) {
        if (!e->is_valid()) continue;
        if (e->dormant()) continue;

        if (valid_target(e)) {
            auto pos = math::Vector::invalid();
            if (visible_target(e, pos)) {
                finished_target(Target{e, pos});

                // Now that we have a target break!
                // TODO: only do this when we want to do speedy targets!
                //break;
            }
        }
    }

    sort_targets();
}

// TODO: move this outside of aimbot
// Other modules may mess the angles up aswell
// So our best bet is to run this at the end of createmove...
inline static auto clamp_angle(const math::Vector &angles) {
    math::Vector out;

    out.x = angles.x;
    out.y = angles.y;
    out.z = 0;

    while (out.x > 89.0f) out.x -= 180.0f;
    while (out.x < -89.0f) out.x += 180.0f;

    while (out.y > 180.0f) out.y -= 360.0f;
    while (out.y < -180.0f) out.y += 360.0f;

    out.y = std::clamp(out.y, -180.0f, 180.0f);
    out.x = std::clamp(out.x, -90.0f, 90.0f);

    return out;
}

inline static auto fix_movement_for_new_angles(const math::Vector &movement, const math::Vector &old_angles, const math::Vector &new_angles) {
    profiler_profile_function();

    math::Matrix3x4 rotate_matrix;

    auto delta_angles = new_angles - old_angles;
    delta_angles      = clamp_angle(delta_angles);

    rotate_matrix.from_angle(delta_angles);
    return rotate_matrix.rotate_vector(movement);
}

// TODO: something something zoomed only convar
static auto try_autoshoot(sdk::UserCmd *cmd) {
    auto autoshoot_allowed = false;

    // Only allow autoshoot when we are zoomed and can get headshots
    if (local_weapon->client_class()->class_id == class_id::CTFSniperRifle && (local_player->cond() & 2)) {
        auto player_time = local_player->tick_base() * IFace<Globals>()->interval_per_tick;
        auto time_delta  = player_time - local_player->fov_time();

        if (time_delta >= 0.2) autoshoot_allowed = true;
    }

    if (autoshoot_allowed) cmd->buttons |= 1;
}

static Convar<bool> doghook_aimbot_silent                       = Convar<bool>{"doghook_aimbot_silent", true, nullptr};
static Convar<bool> doghook_aimbot_autoshoot                    = Convar<bool>{"doghook_aimbot_autoshoot", true, nullptr};
static Convar<bool> doghook_aimbot_aim_if_not_attack            = Convar<bool>{"doghook_aimbot_aim_if_not_attack", true, nullptr};
static Convar<bool> doghook_aimbot_disallow_attack_if_no_target = Convar<bool>{"doghook_aimbot_disallow_attack_if_no_target", false, nullptr};

void create_move(sdk::UserCmd *cmd) {
    profiler_profile_function();

    if (local_weapon == nullptr || !can_find_targets) return;

    find_targets();

    if (doghook_aimbot_aim_if_not_attack != true) {
        // check if we are IN_ATTACK
        if ((cmd->buttons & 1) != 1) return;
    }

    if (targets.size() > 0 && targets[0].first != nullptr) {
        IFace<DebugOverlay>()->add_box_overlay(targets[0].second, {-2, -2, -2}, {2, 2, 2}, {0, 0, 0}, 255, 255, 0, 100, 0);

        math::Vector delta      = targets[0].second - local_view;
        math::Vector new_angles = delta.to_angle();
        new_angles              = clamp_angle(new_angles);

        math::Vector new_movement = fix_movement_for_new_angles({cmd->forwardmove, cmd->sidemove, 0}, cmd->viewangles, new_angles);

        if (local_weapon->can_shoot(local_player->tick_base())) {
            cmd->viewangles = new_angles;

            if (doghook_aimbot_autoshoot == true) try_autoshoot(cmd);

            cmd->forwardmove = new_movement.x;
            cmd->sidemove    = new_movement.y;

            logging::msg("cmd_delta = %d", cmd_delta);
            cmd->tick_count -= cmd_delta;
        }

        if (doghook_aimbot_silent == false) IFace<Engine>()->set_view_angles(new_angles);
    } else {
        if (doghook_aimbot_disallow_attack_if_no_target == true) cmd->buttons &= ~1;
    }
}

void create_move_pre_predict(sdk::UserCmd *cmd) {
    profiler_profile_function();
    // deal with some local data that we want to keep around
    local_player = Player::local();
    assert(local_player);

    if (local_player->alive() == false) {
        can_find_targets = false;
        return;
    }

    local_weapon = local_player->active_weapon()->to_weapon();

    local_team = local_player->team();
    local_view = local_player->view_position();

    // If we dont have the necessary information (we havent spawned yet or are dead)
    // then do not attempt to find targets.
    can_find_targets = local_weapon != nullptr;

    targets.clear();
    // TODO: should we reserve here?
}
} // namespace aimbot

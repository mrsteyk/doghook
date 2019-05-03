#include <precompiled.hh>

#include <algorithm>
#include <queue>

#include "backtrack.hh"

#include <sdk/convar.hh>
#include <sdk/log.hh>
#include <sdk/player.hh>
#include <sdk/sdk.hh>
#include <utils/math.hh>
#include <utils/profiler.hh>

using namespace sdk;

namespace backtrack {

// TODO: remove
// used for hitbox colors...
static math::Vector hullcolor[8] = {
    math::Vector(1.0, 1.0, 1.0),
    math::Vector(1.0, 0.5, 0.5),
    math::Vector(0.5, 1.0, 0.5),
    math::Vector(1.0, 1.0, 0.5),
    math::Vector(0.5, 0.5, 1.0),
    math::Vector(1.0, 0.5, 1.0),
    math::Vector(0.5, 1.0, 1.0),
    math::Vector(1.0, 1.0, 1.0),
};

void draw_entity_bone_transforms(Player *p, const math::Matrix3x4 *bone_to_world) {
    auto model = p->studio_model();

    auto hitbox_set_ptr = model->hitbox_set(0);
    assert(hitbox_set_ptr);

    // Allow us to use operator[] properly
    auto &hitbox_set = *hitbox_set_ptr;

    auto hitboxes_count = std::min(128u, hitbox_set.hitboxes_count);

    for (u32 i = 0; i < hitboxes_count; ++i) {
        auto box = hitbox_set[i];
        assert(box);

        const auto &transform = bone_to_world[box->bone];

        math::Vector origin;
        math::Vector angles;
        math::matrix_angles(transform, angles, origin);

        auto r = 0;
        auto g = 255;
        auto b = 0;

        iface::overlay->add_box_overlay(origin, box->min, box->max, angles, r, g, b, 100, 0);
    }
}

enum { backtrack_max_anim_layers = 15 };

class Record {
public:
    // animation related
#if 0
	std::array<AnimationLayer, backtrack_max_anim_layers> animation_layers;
    std::array<float, 15>                                 pose_parameters;
#endif

    float master_cycle;
    int   master_sequence;

    // state related
    bool alive;
    bool origin_changed;
    bool angles_changed;
    bool size_changed;

    // absolute related
    math::Vector origin;
    math::Vector render_origin;

#if 0
    math::Vector angles;
    math::Vector real_angles;
    math::Vector min_prescaled;
    math::Vector max_prescaled;
#endif

    PlayerHitboxes hitboxes;
    u32            max_hitboxes;

    // misc
    u8      life_state;
    float   simulation_time;
    float   animation_time;
    float   choked_time;
    int     choked_ticks;
    UserCmd user_cmd;
    int     this_tick;

    Record() {
    }

    Record(const Record &other) { std::memcpy(this, &other, sizeof(Record)); }
    auto operator=(const Record &other) { std::memcpy(this, &other, sizeof(Record)); }

    // TODO: this is a nasty hack becuase hitbox registeration will happen before createmove gets called
    // This can be done better and should be done better
    auto reset() {
        std::memset(this, 0, sizeof(Record));
    }
};

enum { backtrack_max_players = 33 };

// Conversion from ticks -> indexes (using rolling array)

auto tick_to_index(u32 tick) { return tick % backtrack::max_ticks; }

// Entites start at index 1 (World is 0)
auto entity_index_to_array_index(u32 index) { return index - 1; }

std::array<std::array<Record, backtrack::max_ticks>, backtrack_max_players> backtrack_records;

// Get the record track (/ record) for an entity (at a tick)
auto &record_track(u32 index) { return backtrack_records[index]; }
auto &record(u32 index, u32 tick) { return record_track(index)[tick_to_index(tick)]; }

// Convenience to get the current data for an entity
// current_tick is updated in create_move_pre_predict
u32   current_tick = 0;
auto &current_record(u32 index) { return record(index, current_tick); }

// Players that have been moved this tick and need restoring
std::vector<sdk::Player *> players_to_restore;

// Handle extended backtrack range using sequence shifting
Convar<float> doghook_backtrack_latency{"doghook_backtrack_latency", 0, 0, 1, nullptr};

// Sequence record
struct sequence {
    u32 in_state;
    u32 out_state;

    u32   in_sequence;
    u32   out_sequence;
    float cur_time;
};

// Recent sequences
std::deque<sequence> sequences;
u32                  last_incoming_sequence;

void add_latency_to_netchannel(NetChannel *c) {
    float current_time = iface::globals->realtime;
    for (auto &s : sequences) {
        if (current_time - s.cur_time > doghook_backtrack_latency) {
            c->in_reliable_state() = s.in_state;
            c->in_sequence()       = s.in_sequence;

            break;
        }
    }
}

void update_incoming_sequences() {
    NetChannel *c = iface::engine->net_channel_info();

    auto incoming_sequence = c->in_sequence();

    if (incoming_sequence > last_incoming_sequence) {
        last_incoming_sequence = incoming_sequence;

        sequences.push_front({c->in_reliable_state(), c->out_reliable_state(), c->in_sequence(), c->out_sequence(), iface::globals->realtime});
    }

    if (sequences.size() > 2048) sequences.pop_back();
}

auto latency_outgoing    = 0.0f;
auto latency_incoming    = 0.0f;
auto total_latency_time  = 0.0f;
u32  total_latency_ticks = 0;

sdk::ConvarWrapper cl_interp{"cl_interp"};

sdk::ConvarWrapper cl_interp_ratio{"cl_interp_ratio"};
sdk::ConvarWrapper cl_update_rate{"cl_updaterate"};

sdk::ConvarWrapper sv_client_min_interp_ratio{"sv_client_min_interp_ratio"};
sdk::ConvarWrapper sv_client_max_interp_ratio{"sv_client_max_interp_ratio"};

sdk::ConvarWrapper sv_minupdaterate{"sv_minupdaterate"};
sdk::ConvarWrapper sv_maxupdaterate{"sv_maxupdaterate"};

sdk::ConvarWrapper sv_maxunlag{"sv_maxunlag"};

float lerp_time() {
    auto interp_ratio = std::clamp(cl_interp_ratio.get_float(), sv_client_min_interp_ratio.get_float(), sv_client_max_interp_ratio.get_float());
    auto update_rate  = std::clamp(cl_update_rate.get_float(), sv_minupdaterate.get_float(), sv_maxupdaterate.get_float());

    auto lerp_time = std::max(interp_ratio / update_rate, cl_interp.get_float());

    return lerp_time;
}

bool tick_valid(u32 tick) {
    auto net_channel = iface::engine->net_channel_info();

    auto lerp       = lerp_time();
    auto lerp_ticks = iface::globals->time_to_ticks(lerp);

    auto correct = std::clamp(lerp + doghook_backtrack_latency + latency_outgoing, 0.0f, sv_maxunlag.get_float());

    auto delta_time = correct - iface::globals->ticks_to_time(iface::globals->tickcount + 1 + lerp_ticks - tick);

    bool valid = std::abs(delta_time) <= 0.2;

    if (!valid) {
        auto new_tick = iface::globals->tickcount - iface::globals->time_to_ticks(correct);
        //logging::msg("[Backtracking] !valid (%d -> %d d: %d)", tick, new_tick, new_tick - tick);
    }

    return valid;
}

// Update player to this record
static bool restore_player_to_record(sdk::Player *p, const Record &r) {
    profiler_profile_function();

    // TODO: now that i have figured out how tracerays work against studiomodels
    // and baseanimatings... Most of these dont matter

    p->set_origin(r.origin);
    p->render_origin() = r.render_origin;
#if 0
    p->set_angles(r.angles);
    //p->render_angle() = r.angles;
#endif

    p->sim_time()  = r.simulation_time;
    p->anim_time() = r.animation_time;

    p->cycle()    = r.master_cycle;
    p->sequence() = r.master_sequence;

#if 0

    auto layer_count = p->anim_layer_count();

    auto layer_max = std::min<u32>(backtrack_max_anim_layers, layer_count);

    // Copy animation layers across
    for (u32 i = 0; i < layer_max; ++i) {
        p->anim_layer(i) = r.animation_layers[i];
    }

    // Because we changed the animation we need to tell the
    // animation state about it
    p->update_client_side_animation();
#endif

    // Now we need to tell the cache that these bones are different...

    // The C_BaseAnimating::TestHitboxes will still try and get the bones from yet another, different, cache...
    // So we need to effectively deal with that aswell

    struct BoneCache;
    using GetBoneCacheFn          = BoneCache *(*)(u32);
    using BoneCache_UpdateBonesFn = void(__thiscall *)(BoneCache *, const math::Matrix3x4 *, u32, float);

#if doghook_platform_windows()

    // This offset already seems to account for the + 4 needed...
    static auto hitbox_bone_cache_handle_offset = *signature::find_pattern<u32 *>("client", "FF B6 ? ? ? ? E8 ? ? ? ? 8B F8 83 C4 04 85 FF 74 47", 2);
    static auto studio_get_bone_cache           = signature::find_pattern<GetBoneCacheFn>("client", "55 8B EC 83 EC 20 56 6A 01 68 ? ? ? ? 68", 0);
    static auto bone_cache_update_bones         = signature::find_pattern<BoneCache_UpdateBonesFn>("client", "55 8B EC 83 EC 08 56 8B F1 33 D2", 0);

#else
    // 8B 86 ? ? ? ? 89 04 24 E8 ? ? ? ? 85 C0 89 C3 74 48 -> hitbox_bone_cache_handle_offset
    // 55 89 E5 56 53 BB ? ? ? ? 83 EC 50 C7 45 D8 -> GetBoneCache
    // 55 89 E5 57 31 FF 56 53 83 EC 1C 8B 5D 08 0F B7 53 10  -> UpdateBoneCache

    static auto hitbox_bone_cache_handle_offset = *signature::find_pattern<u32 *>("client", "8B 86 ? ? ? ? 89 04 24 E8 ? ? ? ? 85 C0 89 C3 74 48", 2);
    static auto studio_get_bone_cache           = signature::find_pattern<GetBoneCacheFn>("client", "55 89 E5 56 53 BB ? ? ? ? 83 EC 50 C7 45 D8", 0);
    static auto bone_cache_update_bones         = signature::find_pattern<BoneCache_UpdateBonesFn>("client", "55 89 E5 57 31 FF 56 53 83 EC 1C 8B 5D 08 0F B7 53 10", 0);
#endif

    auto hitbox_bone_cache_handle = p->get<u32>(hitbox_bone_cache_handle_offset);
    if (hitbox_bone_cache_handle == 0) return true;

    BoneCache *bone_cache = studio_get_bone_cache(hitbox_bone_cache_handle);

    if (bone_cache != nullptr) bone_cache_update_bones(bone_cache, r.hitboxes.bone_to_world, 128, iface::globals->curtime);

    return true;
}

// Backtrack a player to this tick
bool backtrack_player_to_tick(sdk::Player *p, u32 tick, bool set_alive, bool restoring) {
    profiler_profile_function();

    auto array_index = entity_index_to_array_index(p->index());

    auto &r = record(array_index, tick);

    // if (set_alive) p->life_state() = r.life_state;
    if (r.alive == false) return false;

    if (!restoring) players_to_restore.push_back(p);

    bool success = restore_player_to_record(p, r);

    return success;
}

// Set the hitboxes for this player at this tick
void update_player_hitboxes(sdk::Player *p, const sdk::PlayerHitboxes &hitboxes, u32 hitboxes_count) {
    profiler_profile_function();

    auto &record = current_record(entity_index_to_array_index(p->index()));

    std::memcpy(&record.hitboxes, &hitboxes, sizeof(PlayerHitboxes));
    record.max_hitboxes = hitboxes_count;
}

// Get the hitboxes for this player at this tick
// TODO: this can probably just return a const pointer to the hitboxes inside the record
u32 hitboxes_for_player(sdk::Player *p, u32 tick, sdk::PlayerHitboxes &hitboxes) {
    profiler_profile_function();

    auto array_index = entity_index_to_array_index(p->index());

    auto &r = record(array_index, tick);

    if (!r.alive) return 0;

    std::memcpy(&hitboxes, &r.hitboxes, sizeof(PlayerHitboxes));

    return r.max_hitboxes;
}

void update_incoming_sequences();

// Get new information about each player
void create_move_pre_predict(sdk::UserCmd *cmd) {
    profiler_profile_function();

    update_incoming_sequences();

    current_tick = iface::globals->tickcount;

    auto local_player = Player::local();

    // Get the current latency (not useful here but will be later on in the tick)
    latency_incoming    = iface::engine->net_channel_info()->latency(NetChannel::Flow::incoming);
    latency_outgoing    = iface::engine->net_channel_info()->latency(NetChannel::Flow::outgoing);
    total_latency_time  = latency_incoming + latency_outgoing;
    total_latency_ticks = iface::globals->time_to_ticks(total_latency_time);

    // We want to get from entity 1 to entity 32
    for (auto entity : iface::ent_list->get_range(iface::engine->max_clients() + 1)) {
        if (entity == nullptr) continue;

        auto player = entity->to_player();
        if (player == nullptr || player == local_player) continue;

        if (player->team() == local_player->team()) continue;

        // Get the record for this tick and clear it out
        auto  array_index     = entity_index_to_array_index(entity->index());
        auto &new_record      = record(array_index, current_tick);
        auto &previous_record = record(array_index, current_tick - 1);

        // Clean out the record - might not be necessary but a memset is pretty cheap
        new_record.reset();

        new_record.life_state = player->life_state();
        new_record.alive      = player->alive();
        if (player->dormant()) new_record.alive = false;

        // If this fails then it is clear to other algorithms whether this record is valid just by looking at this bool
        if (!new_record.alive) continue;

        profiler_profile_scope("update_record");

        // TODO: look above at restore_player_to_tick for information about why this
        // is commented out.

        // Set absolute origin and angles
        new_record.origin        = player->origin();
        new_record.render_origin = player->render_origin();
#if 0
        new_record.angles = player->angles();

        // TODO: when real angles and corrected angles differ we need to change this
        new_record.real_angles = player->angles();

        // TODO: min_prescaled, max_prescaled

        new_record.origin_changed = previous_record.origin != new_record.origin;
        new_record.angles_changed = previous_record.angles != new_record.angles;
        new_record.size_changed   = previous_record.min_prescaled != new_record.min_prescaled ||
                                  previous_record.max_prescaled != new_record.max_prescaled;

#endif

        new_record.simulation_time = player->sim_time();
        new_record.animation_time  = player->anim_time();

        // TODO: calculate choked ticks...

        new_record.master_cycle    = player->cycle();
        new_record.master_sequence = player->sequence();
        new_record.this_tick       = current_tick;

#if 0

        auto layer_count = player->anim_layer_count();
#ifdef _DEBUG
        if (layer_count > backtrack_max_anim_layers)
            logging::msg("Not enough space for all layers (has %d, needs %d)",
                         backtrack_max_anim_layers, layer_count);
#endif

        auto layer_max = std::min<u32>(backtrack_max_anim_layers, layer_count);
        for (u32 i = 0; i < layer_max; ++i) {
            new_record.animation_layers[i] = player->anim_layer(i);
        }
#endif

        new_record.max_hitboxes = player->hitboxes(&new_record.hitboxes, false);
    }
}

void create_move(sdk::UserCmd *cmd) {
#if doghook_platform_debug()
    profiler_profile_function();

    for (auto entity : iface::ent_list->get_range()) {
        if (!entity->is_valid()) continue;

        auto player = entity->to_player();
        if (auto player = entity->to_player()) {
            for (auto &r : record_track(player->index())) {
                //auto &r = record(player->index(), iface::Globals->tickcount + 1);

                if (!r.alive) continue;

                if (!tick_valid(r.this_tick)) continue;

                iface::overlay->add_box_overlay(r.hitboxes.origin[0], {-2, -2, -2}, {2, 2, 2}, {0, 0, 0}, 0, 255, 0, 100, 0);

#if 0 && _DEBUG
                auto &hitboxes = r.hitboxes;
                for (u32 i = 0; i < r.max_hitboxes; i++) {

                    auto j = (i % 8);
                    auto r = (int)(255.0f * hullcolor[j].x);
                    auto g = (int)(255.0f * hullcolor[j].y);
                    auto b = (int)(255.0f * hullcolor[j].z);

                    iface::overlay->add_box_overlay(hitboxes.origin[i], hitboxes.raw_min[i], hitboxes.raw_max[i], hitboxes.rotation[i], r, g, b, 100, 0);

                    //math::Vector origin;
                    //math::Vector angles;
                    //math::matrix_angles(bone_transform, angles, origin);

                    //r = 0;
                    //g = 255;
                    //b = 0;

                    //iface::overlay->add_box_overlay(origin, hitboxes.raw_min[i], hitboxes.raw_max[i], angles, r, g, b, 100, 0);
                }
#endif
            }
        }
    }
#endif
}

void restore_all_players() {
    profiler_profile_function();

    for (auto &p : players_to_restore) backtrack_player_to_tick(p, current_tick, true, true);

    players_to_restore.clear();
}

void create_move_finish(sdk::UserCmd *cmd) {
    // Cleanup from whatever has been done
    restore_all_players();
}

bool rewind_state_active = false;
RewindState::RewindState() {
    assert(!rewind_state_active);

    rewind_state_active = true;
}

RewindState::~RewindState() {
    restore_all_players();

    rewind_state_active = false;
}

// TODO: if a player isnt alive at a record then we need a way
// of effectively eliminating their bones from traceray
void RewindState::to_tick(u32 t) {
    // TODO: could this be multithreaded effectively?

    auto local_player = Player::local();

    for (auto entity : iface::ent_list->get_range(iface::engine->max_clients() + 1)) {
        if (!entity->is_valid()) continue;

        if (auto p = entity->to_player()) {
            if (p == local_player) continue;

            if (p->team() == local_player->team()) continue;

            // Setalive so that dead players at this tick are dead...
            // This means that aimbots is_valid will work properly
            auto success = backtrack_player_to_tick(p, t, true);

            if (!success) {
                // Player isnt valid this tick...
                // TODO: Set hitboxes to something invalid here...
            }
        }
    }
}
} // namespace backtrack

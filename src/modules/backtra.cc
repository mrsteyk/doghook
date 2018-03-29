#include <precompiled.hh>

#include "backtrack.hh"

#include <sdk/log.hh>
#include <sdk/player.hh>
#include <sdk/sdk.hh>
#include <utils/math.hh>

using namespace sdk;

namespace backtrack {

enum { backtrack_max_anim_layers = 15 };

class Record {
public:
    // animation related
    std::array<AnimationLayer, backtrack_max_anim_layers> animation_layers;
    std::array<float, 15>                                 pose_parameters;
    float                                                 master_cycle;
    int                                                   master_sequence;

    // state related
    bool alive;
    bool origin_changed;
    bool angles_changed;
    bool size_changed;

    // absolute related
    math::Vector origin;
    math::Vector render_origin;
    math::Vector angles;
    math::Vector real_angles;
    math::Vector min_prescaled;
    math::Vector max_prescaled;

    PlayerHitboxes hitboxes;
    u32            max_hitboxes;

    // misc
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
// and entity_index -> index (subtract 1 as world is eindex 0)

auto tick_to_index(u32 tick) { return tick % backtrack::max_ticks; }
auto entity_index_to_array_index(u32 index) { return index - 1; }

std::array<std::array<Record, backtrack::max_ticks>, backtrack_max_players> backtrack_records;

auto &record_track(u32 index) { return backtrack_records[index]; }
auto &record(u32 index, u32 tick) { return record_track(index)[tick_to_index(tick)]; }

u32   current_tick = 0;
auto &current_record(u32 index) { return record(index, current_tick); }

// Players that have been moved this tick and need restoring
std::vector<sdk::Player *> players_to_restore;

// Update player to this record
static bool restore_player_to_record(sdk::Player *p, const Record &r) {
    p->set_origin(r.origin);
    p->render_origin() = r.render_origin;
    p->set_angles(r.angles);

    p->sim_time()  = r.simulation_time;
    p->anim_time() = r.animation_time;

    p->cycle()    = r.master_cycle;
    p->sequence() = r.master_sequence;

    auto layer_count = p->anim_layer_count();
    auto layer_max   = std::min<u32>(backtrack_max_anim_layers, layer_count);

    // Copy animation layers across
    for (u32 i = 0; i < layer_max; ++i) {
        p->anim_layer(i) = r.animation_layers[i];
    }

    // Because we changed the animation we need to tell the
    // animation state about it
    p->update_client_side_animation();

    // Make sure to clean out the bone cache so that setupbones returns new values
    // TODO: We probably dont need to do this becuase we are already have hitboxes for this tick...
#if doghook_platform_windows()
    static auto g_iModelBoneCounter = *(signature::find_pattern<long **>("client", "A1 ? ? ? ? D9 45 0C", 1));
    (*g_iModelBoneCounter)++;
#endif

    return true;
}

// Backtrack a player to this tick
bool backtrack_player_to_tick(sdk::Player *p, u32 tick, bool restoring) {
    auto array_index = entity_index_to_array_index(p->index());

    auto &r = record(array_index, tick);
    if (r.alive == false) return false;

    if (restoring == false) players_to_restore.push_back(p);
    return restore_player_to_record(p, r);
}

// Set the hitboxes for this player at this tick
void update_player_hitboxes(sdk::Player *p, const sdk::PlayerHitboxes &hitboxes, u32 hitboxes_count) {
    auto &record = current_record(entity_index_to_array_index(p->index()));

    std::memcpy(&record.hitboxes, &hitboxes, sizeof(PlayerHitboxes));
    record.max_hitboxes = hitboxes_count;
}

// Get the hitboxes for this player at this tick
// TODO: this can probably just return a const pointer to the hitboxes inside the record
void hitboxes_for_player(sdk::Player *p, u32 tick, sdk::PlayerHitboxes &hitboxes) {
    auto array_index = entity_index_to_array_index(p->index());

    auto &r = record(array_index, tick);

    if (r.alive == false) return;

    std::memcpy(&hitboxes, &r.hitboxes, sizeof(PlayerHitboxes));
}

// Get new information about each player
void create_move_pre_predict(sdk::UserCmd *cmd) {
    // This can be called anywhere in the createmove function
    // Because either modules can use the current up to date data
    // or they will backtrack to a previous tick that isnt this current one.
    current_tick = IFace<Globals>()->tickcount;

    auto local_player = Player::local();

    // We want to get from entity 1 to entity 32
    for (auto entity : IFace<EntList>()->get_range(IFace<Engine>()->max_clients() + 1)) {
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

        new_record.alive = player->alive();
        if (player->dormant() == true) new_record.alive = false;

        // If this fails then it is clear to other algorithms whether this record is valid just by looking at this bool
        if (new_record.alive == false) continue;

        // Set absolute origin and angles
        new_record.origin        = player->origin();
        new_record.render_origin = player->render_origin();
        new_record.angles        = player->angles();

        // TODO: when real angles and corrected angles differ we need to change this
        new_record.real_angles = player->angles();

        // TODO: min_prescaled, max_prescaled

        new_record.origin_changed = previous_record.origin != new_record.origin;
        new_record.angles_changed = previous_record.angles != new_record.angles;
        new_record.size_changed   = previous_record.min_prescaled != new_record.min_prescaled ||
                                  previous_record.max_prescaled != new_record.max_prescaled;

        new_record.simulation_time = player->sim_time();
        new_record.animation_time  = player->anim_time();

        // TODO: calculate choked ticks...

        new_record.master_cycle    = player->cycle();
        new_record.master_sequence = player->sequence();
        new_record.this_tick       = current_tick;

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

        // TODO: pose parameters (are these even important??)
    }
}

// TODO: remove
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

void create_move(sdk::UserCmd *cmd) {
#ifdef _DEBUG
    for (auto entity : IFace<EntList>()->get_range()) {
        if (entity->is_valid() == false) continue;
        auto player = entity->to_player();
        if (player == nullptr) continue;

        auto &track = record_track(entity_index_to_array_index(player->index()));

        for (auto &record : track) {
            if (record.alive == false) break;

            IFace<DebugOverlay>()->add_text_overlay(record.origin, 0, "%d", record.this_tick);

            //if (record.this_tick == cmd->tick_count) {
            auto &hitboxes = record.hitboxes;
            for (u32 i = 0; i < record.max_hitboxes; i++) {

                auto j = (record.this_tick % 8);
                auto r = (int)(255.0f * hullcolor[j].x);
                auto g = (int)(255.0f * hullcolor[j].y);
                auto b = (int)(255.0f * hullcolor[j].z);

                IFace<DebugOverlay>()->add_box_overlay(hitboxes.origin[i], hitboxes.raw_min[i], hitboxes.raw_max[i], hitboxes.rotation[i], r, g, b, 100, 0);
            }
            //}
        }
    }
#endif
}

auto create_move_finish(sdk::UserCmd *cmd) -> void {
    // Cleanup from whatever has been done
    for (auto &p : players_to_restore) {
        backtrack_player_to_tick(p, current_tick, true);
    }

    players_to_restore.clear();
}
} // namespace backtrack

#include <precompiled.hh>

#include <sdk/class_id.hh>
#include <sdk/convar.hh>
#include <sdk/hooks.hh>
#include <sdk/log.hh>
#include <sdk/sdk.hh>

#include <inttypes.h>
#include <utility>
#include <vector>

using namespace sdk;

class CClientState;

namespace process_temp_entities {

std::unique_ptr<hooks::HookFunction<CClientState, 2>> process_temp_entities_hook;
std::unique_ptr<hooks::HookFunction<Engine, 0>>       cl_fire_event_hook;

constexpr size_t max_seeds    = 0xFFFF;
size_t           curr_seed_id = 0;

struct seed_struct {
    int    tick_count;
    u32    seed;
    double time;
    bool   operator<(const seed_struct &rhs) const {
        return tick_count < rhs.tick_count;
    }
};

std::array<seed_struct, max_seeds> seeds;
std::vector<seed_struct>           seeds_rebased;
seed_struct                        seed_selected;
std::vector<std::pair<int, float>> intervals; // pos, val

constexpr float MIN_CLOCKRES = 0.25;
constexpr float MAX_CLOCKRES = 8192.5;
float           clock_res;
float           seed_fraction = 0.0f;

float predict_offset(const seed_struct &entry, u32 target_tick, float clock_res) {
    return (1000.0f * iface::globals->interval_per_tick / clock_res) * (float)(target_tick - entry.tick_count);
}

int predict_seed(const seed_struct &entry, int target_tick, float clock_res, float seed_offset) {
    return (entry.seed + (int)((predict_offset(entry, target_tick, clock_res)) + seed_offset)) % 256;
}

int predictSeed(float target_time) {
    auto   ch                 = iface::engine->net_channel_info();
    auto   latency_incoming   = iface::engine->net_channel_info()->latency(NetChannel::Flow::incoming);
    auto   latency_outgoing   = iface::engine->net_channel_info()->latency(NetChannel::Flow::outgoing);
    double total_latency_time = (double)latency_incoming + latency_outgoing; // to shut up warnings about 4 and 8 byte vals
    float  ping               = total_latency_time / 2;
    float  delta_time         = target_time - seed_selected.time + ping;
    int    tick               = (int)((float)(seed_selected.tick_count) + delta_time / iface::globals->interval_per_tick + 0.7);
    float  seed_offset        = predict_offset(seed_selected, tick, clock_res);
    int    seed               = predict_seed(seed_selected, tick, clock_res, seed_offset);

    logging::msg("Seed pred: guessed tick = %d, guessed seed = %03d", tick, seed);
    return seed;
}

void select_base() {
    if (curr_seed_id <= 1) {
        return;
    }
    size_t total = curr_seed_id >= max_seeds ? max_seeds : curr_seed_id;
    //size_t total  = curr_seed_id % max_seeds;
    seed_selected = seeds[total - 1];
    seed_fraction = 0.f;

    // Algorithmic approach to estimate server time offset

    // 1. Find clock resolution
    // For each reasonable precision value "rebase" seeds to the same tick
    // and check if they are close to each other (by looking for largest gap
    // between).

    int   best_gap      = 0;
    float new_clock_res = 1.f;

    for (float res = MIN_CLOCKRES; res < MAX_CLOCKRES + 1.f; res *= 2.f) {
        seeds_rebased.clear();
        for (size_t i = 0; i < total; i++) {
            seeds_rebased.push_back(seeds[i]);
        }

        std::sort(seeds_rebased.begin(), seeds_rebased.end());
        int gap = 0;

        for (size_t i = 0; i < seeds_rebased.size(); i++) {
            auto left  = seeds_rebased[i].tick_count;
            auto right = seeds_rebased[(i + 1) < seeds_rebased.size() ? (i + 1) : 0].tick_count;
            gap        = std::max(gap, (right - left) % 256);
        }

        gap = (gap > 0 ? gap : 256);
        if (best_gap < gap) {
            best_gap      = gap;
            new_clock_res = res;
        }
    }

    if (total >= 5) {
        clock_res = new_clock_res;
    }

    // 2. Find seed fraction offset
    // Estimate time more precisely: "rebase" seeds to same tick (keep fraction
    // part),
    // interpret them as intervals of size 1 and find offset which covers most
    // of them.

    float max_disp = 5.0 / clock_res;
    intervals.clear();

    for (size_t i = 0; i < total; i++) {
        float disp = (float)seeds[i].seed - (float)seed_selected.seed;
        disp       = fmod(disp - predict_offset(seed_selected, seeds[i].tick_count, clock_res), 256);
        disp       = (disp > 128.f ? disp - 256.f : disp);

        if (abs(disp) < fmaxf(1.2, max_disp)) {
            intervals.push_back(std::make_pair<int, float>(1, disp - 0.5f));
            intervals.push_back(std::make_pair<int, float>(-1, disp + 0.5f));
        }
    }

    int cur_chance = 0, best_chance = 0;
    std::sort(intervals.begin(), intervals.end(), [](const std::pair<int, float> &x, const std::pair<int, float> &y) { return x.second < y.second; });

    for (size_t i = 0; (i + 1) < intervals.size(); i++) {
        auto &inter = intervals[i];
        cur_chance += (int)inter.second;

        if (cur_chance > best_chance) {
            best_chance   = cur_chance;
            seed_fraction = (inter.first + intervals[i + 1].first) / 2.f;
        }
    }

    logging::msg("Seed pred: res=%.3f | chance = %d", clock_res, best_chance * 100 / total);
}

void handle_fire_bullets(u32 seed) {
    auto   latency_incoming         = iface::engine->net_channel_info()->latency(NetChannel::Flow::incoming);
    auto   latency_outgoing         = iface::engine->net_channel_info()->latency(NetChannel::Flow::outgoing);
    double total_latency_time       = (double)latency_incoming + latency_outgoing; // to shut up warnings about 4 and 8 byte vals
    double time                     = iface::globals->tickcount * (double)iface::globals->interval_per_tick - total_latency_time / 2;
    seeds[curr_seed_id % max_seeds] = {iface::globals->tickcount, seed, time};
    curr_seed_id++;
    select_base();
}

// TODO: find out why head is fucked up!

#if doghook_platform_windows()
bool __fastcall hooked_process_temp_entities(CClientState *this_ptr, void *, uptr msg)
#else
bool hooked_process_temp_entities(CClientState *this_ptr, uptr msg)
#endif
{
    auto ret = process_temp_entities_hook->call_original<bool>(msg);

    // We should iterate over events here! or do we???
    // Now that Tom(as) implemented it in his own cheat, will he tell what he used???
    // TODO: find out why his cheat had references to "m_iSeed" and "DT_FireBullets", or how that shits called, prior to todays release
#if doghook_platform_windows()
    static auto cl_event_head = *(u8 ***)(signature::find_pattern("engine", "8B 1D ? ? ? ? 89 5D FC") + 2);
#else
#error "Implement me"
#endif
    auto cur_elem = *cl_event_head;
    while (cur_elem) {
        if (*(u16 *)cur_elem == 0) {
            cur_elem = *(u8 **)(cur_elem + 15 * 4);
            continue;
        }
        auto cclass = *(ClientClass **)(cur_elem + 3 * 4);
        if (cclass->class_id == class_id::CTEFireBullets) {
            static u32 prev_seed = 0;
            //TODO: do recvtable bs instead of this bs which doesnt works
            u32 cur_seed = *(u32 *)(*(u8 **)(cur_elem + 5 * 4) + 0x34);  // i think im dumb and its not like that TODO: FIX FIX FIXXXXX FIX FIX
                                                                         //if (prev_seed != cur_seed) {               // should be enough to check for dups and some bad bs
            logging::msg("TEFireBullet! m_iSeed is %" PRIu32, cur_seed); // TODO: make delta??? seed! (if seed==0 then its prev_seed ???) or can i actually ignore duplicates??? i also got negatives wtf
            handle_fire_bullets(cur_seed);
            prev_seed = cur_seed;
        }
        //logging::msg("[FireEvent] Event: %d | name: %s", cclass->class_id, cclass->network_name);
        cur_elem = *(u8 **)(cur_elem + 15 * 4);
    }

    return ret;
}

/*#if doghook_platform_windows()
void __fastcall hooked_cl_fire_event(Engine *this_ptr, void *)
#else
void hooked_cl_fire_event(Engine *this_ptr, )
#endif
{
    // We should iterate over events here!
    static auto cl_event_head = *(u8 ***)(signature::find_pattern("engine", "8B 1D ? ? ? ? 89 5D FC") + 2);
    auto        cur_elem      = *cl_event_head;
    while (cur_elem) {
        if (*(u16 *)cur_elem == 0) {
            cur_elem = *(u8 **)(cur_elem + 15 * 4);
            continue;
        }
        auto cclass = *(ClientClass **)(cur_elem + 3 * 4);
        if (cclass->class_id == class_id::CTEFireBullets) {
            static u32 prev_seed = 0;
            u32        cur_seed  = *(u32 *)(*(u8 **)(cur_elem + 5 * 4) + 0x34);
            if (prev_seed != cur_seed && cur_seed > 0) {              // should be enough to check for dups and some bad bs
                logging::msg("TEFireBullet! m_iSeed is %d", cur_seed); // TODO: make delta??? seed! (if seed==0 then its prev_seed ???) or can i actually ignore duplicates??? i also got negatives wtf
                //seeds[curr_seed_id % max_seeds] = cur_seed;
                //curr_seed_id++;
                handle_fire_bullets(cur_seed);
            }
            prev_seed = cur_seed;
        }
        //logging::msg("[FireEvent] Event: %d | name: %s", cclass->class_id, cclass->network_name);
        cur_elem = *(u8 **)(cur_elem + 15 * 4);
    }

    cl_fire_event_hook->call_original<void>();
}*/

void level_init() {
    return;
    assert(process_temp_entities_hook == nullptr);
    static auto client_state_addr = *(CClientState **)(signature::find_pattern("engine", "B9 ? ? ? ? E8 ? ? ? ? 83 F8 FF 5E") + 1);
    process_temp_entities_hook    = std::make_unique<hooks::HookFunction<CClientState, 2>>(client_state_addr, 24, 0, 0, reinterpret_cast<void *>(&hooked_process_temp_entities));
    //j_CL_FireEvent - 56
    //assert(cl_fire_event_hook == nullptr);
    //cl_fire_event_hook = std::make_unique<hooks::HookFunction<Engine, 0>>(iface::engine, 56, 0, 0, reinterpret_cast<void *>(&hooked_cl_fire_event));
}

void level_shutdown() {
    return;
    process_temp_entities_hook.reset();
    process_temp_entities_hook = nullptr;
    //cl_fire_event_hook.reset();
    //cl_fire_event_hook = nullptr;
    curr_seed_id = 0;
    clock_res    = 0;
}

} // namespace process_temp_entities

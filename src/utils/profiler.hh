#pragma once

#include <vector>

#include "sdk/platform.hh"

namespace profiler {
class Timer {
    static u64   clock_speed;
    static float clock_multiplier_micro;
    static float clock_multiplier_milli;
    static float clock_multiplier_whole;

    u64 begin_count;
    u64 duration;

public:
    // Init for calculating clock speed
    static void calculate_clock_speed();

    u64 sample();

    auto start() { begin_count = sample(); }
    auto end() {
        auto end_count = sample();
        duration       = end_count - begin_count;
    }

    auto reset() { begin_count = 0; }

    auto cycles() { return duration; }
    auto milliseconds() { return float(duration * 1000) / float(clock_speed); }
};

void init();

// Represents the graph of nodes inside the program
// Do not create directly but use ProfileScope
class ProfileNode {
public:
    u32         id;
    const char *name;
    Timer       t;

    u32 recursions;

    ProfileNode *              parent;
    std::vector<ProfileNode *> children;
};

void enter_node(u32 id, const char *name);
void exit_node();

bool profiling_enabled();
void set_profiling_enabled(bool s);

// access all nodes via index (returns nullptr at end of array)
ProfileNode *node(u32 index);
ProfileNode *find_root_node();

class ProfileScope {
public:
    ProfileScope(const char *name) {
        if (!profiling_enabled()) return;

        // assume that each name is unique
        u32 id = reinterpret_cast<u32>(name);
        enter_node(id, name);
    }

    ~ProfileScope() {
        if (!profiling_enabled()) return;
        exit_node();
    }
};

//#ifdef _DEBUG
#if _MSC_VER
#define profiler_profile_function() \
    auto __ProfileScope##__LINE__ = profiler::ProfileScope(__FUNCTION__)
#else
#define profiler_profile_function() \
    auto __ProfileScope##__LINE__ = profiler::ProfileScope(__PRETTY_FUNCTION__)
#endif
#define profiler_profile_scope(name) \
    auto __ProfileScope##__LINE__ = profiler::ProfileScope("/" name)
//#else
//#define profiler_profile_function()
//#define profiler_profile_scope(name)
//#endif

} // namespace profiler

#include "precompiled.hh"

#include "profiler.hh"

#include <stack>
#include <vector>

#if doghook_platform_linux()
#include <sys/time.h>
#include <unistd.h>
#include <x86intrin.h>

class TimeVal {
public:
    TimeVal() {}
    TimeVal &operator=(const TimeVal &other) {
        tv = other.tv;
        return *this;
    }

    inline double operator-(const TimeVal &left) {
        u64 left_us  = (u64)left.tv.tv_sec * 1000000 + left.tv.tv_usec;
        u64 right_us = (u64)tv.tv_sec * 1000000 + tv.tv_usec;
        u64 diff_us  = left_us - right_us;
        return diff_us / 1000000;
    }

    timeval tv;
};

static inline double timeval_sub(const timeval &left, const timeval &right) {
    auto right_us = (u64)right.tv_sec * 1000000 + right.tv_usec;
    auto left_us  = (u64)left.tv_sec * 1000000 + left.tv_usec;

    // dont question it
    auto diff = right_us - left_us;
    return diff / 1000000;
}
// Positive diff of 2 u64s
static inline u64 diff(u64 v1, u64 v2) {
    i64 d = v1 - v2;
    if (d >= 0)
        return d;
    else
        return -d;
}

#endif

namespace profiler {
u64   Timer::clock_speed;
float Timer::clock_multiplier_micro;
float Timer::clock_multiplier_milli;
float Timer::clock_multiplier_whole;

void Timer::calculate_clock_speed() {
#if doghook_platform_windows()
    LARGE_INTEGER waitTime, startCount, curCount;

    Timer t;

    // Take 1/32 of a second for the measurement.
    QueryPerformanceFrequency(&waitTime);
    int scale = 5;
    waitTime.QuadPart >>= scale;

    QueryPerformanceCounter(&startCount);
    t.start();
    {
        do {
            QueryPerformanceCounter(&curCount);
        } while (curCount.QuadPart - startCount.QuadPart < waitTime.QuadPart);
    }
    t.end();

    clock_speed = t.cycles() << scale;
#else
    double mhz = 0;
    char   line[1024], *s, search_str[] = "cpu MHz";
    FILE * fp;

    /* open proc/cpuinfo */
    if ((fp = fopen("/proc/cpuinfo", "r")) == NULL) {
        assert(0);
    }
    defer(fclose(fp));

    /* ignore all lines until we reach MHz information */
    while (fgets(line, 1024, fp) != NULL) {
        if (strstr(line, search_str) != NULL) {
            /* ignore all characters in line up to : */
            for (s = line; *s && (*s != ':'); ++s)
                ;
            /* get MHz number */
            if (*s && (sscanf(s + 1, "%lf", &mhz) == 1))
                break;
        }
    }

    clock_speed = mhz * 1000000;

#endif

    // Deal with the multipliers here aswell...
    clock_multiplier_whole = 1.0f / clock_speed;
    clock_multiplier_milli = 1000.0f / clock_speed;
    clock_multiplier_micro = 1000000.0f / clock_speed;
}

u64 Timer::sample() {
    return __rdtsc();
}

// Calculate the clockspeed on init
init_time(Timer::calculate_clock_speed());

std::vector<ProfileNode *> nodes;

ProfileNode *find_node(u32 id) {
    for (auto &n : nodes)
        if (n->id == id) return n;

    return nullptr;
}

std::stack<ProfileNode *, std::vector<ProfileNode *>> current_nodes;
ProfileNode *                                         root_node;

ProfileNode *find_root_node() {
    return root_node;
}

void init() {
    root_node = new ProfileNode();

    root_node->id = -1;
    root_node->t.start();
    root_node->parent = nullptr;
    root_node->name   = "root_node";

    nodes.push_back(root_node);
}

void enter_node(u32 id, const char *name) {
    if (current_nodes.size() < 1) {
        // we clearly arent inited yet...
        current_nodes.push(find_root_node());
    }

    // TODO: maybe we should try and search for nodes that we know are children first??
    auto current_node = current_nodes.top();
    auto node         = find_node(id);

    if (node == nullptr) {

        node = new ProfileNode();

        node->parent = current_nodes.top();
        node->name   = name;
        node->id     = id;

        node->recursions = 0;

        current_node->children.push_back(node);

        nodes.push_back(node);
    }

    if (node->recursions == 0) {
        node->t.start();
        node->recursions = 1;

        current_nodes.push(node);
    } else {
        node->recursions += 1;
    }
}

void exit_node() {
    if (current_nodes.size() < 1) return; // we clearly arent inited yet...

    auto node = current_nodes.top();

    if (node->recursions == 1) {
        node->recursions = 0;
        current_nodes.pop();

        node->t.end();
    } else {
        node->recursions -= 1;
    }
}

ProfileNode *node(u32 index) {
    if (index < nodes.size()) return nodes[index];

    return nullptr;
}

} // namespace profiler

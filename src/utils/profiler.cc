#include "precompiled.hh"

#include "profiler.hh"

#include <stack>
#include <vector>

#if doghook_platform_linux()
#include <sys/time.h>
#include <unistd.h>
#include <x86intrin.h>
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

#ifdef _DEBUG
bool enable_profiling = true;
#else
bool enable_profiling = false;
#endif

bool profiling_enabled() { return enable_profiling; }
void set_profiling_enabled(bool s) { enable_profiling = s; }

// TODO: using this flat based structure means that we cant have a 2 nodes for the same location
// Take the function draw::test, it may be called from multiple places, but there is only 1 timer for it
// This means that the timer will be wrong for that function / may not show up in the graph properly...
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

    if (!enable_profiling) return;

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

    if (current_node != node)
        // If we arent the child of the current node then add us
        if (std::find(current_node->children.begin(), current_node->children.end(), node) == current_node->children.end()) {
            current_node->children.push_back(node);
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

    if (!enable_profiling) return;

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

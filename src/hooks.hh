#pragma once

#include "types.hh"
#include "vfunc.hh"

#include <algorithm>
#include <utility>
#include <vector>

namespace Hooks {
template <typename T, u32 offset>
class HookInstance {

    T *    instance;
    void **original_table;
    void **new_table;

    // pair from index to old function pointer
    std::vector<std::pair<u32, void *>> hooked_functions;

    auto count_funcs() {
        u32 count = -1;
        do
            ++count;
        while (original_table[count] != nullptr);

        return count;
    }

    auto get_table() {
        return *(reinterpret_cast<void ***>(instance) + offset);
    }

    static auto create_table(u32 count) {
        return new void *[count];
    }

    static auto replace_table_pointer(T *instance, void **new_pointer) {
        auto table = reinterpret_cast<void ***>(instance) + offset;
        *table     = new_pointer;
    }

public:
    HookInstance(T *instance) : instance(instance) {

        assert(instance);

        original_table = get_table();
        assert(original_table);

        auto num_funcs = count_funcs();
        assert(num_funcs != -1);

        new_table = create_table(num_funcs);

        // copy the original function pointers over
        memcpy(new_table, original_table, num_funcs * 4);

        // replace the original with our own
        replace_table_pointer(instance, new_table);
    }

    ~HookInstance() {
        replace_table_pointer(instance, original_table);

        assert(new_table);
        delete[] new_table;
    }

    auto hook_function(u32 index, void *f) {
        assert(index < count_funcs());

        hooked_functions.push_back(std::make_pair(index, f));
        new_table[index] = f;
    }

    auto unhook_function(u32 index) -> void {
        assert(index < count_funcs());

        auto found = false;
        for (auto &f : hooked_functions) {
            if (f.first == index) {
                found = true;

                // remove from the hooked functions list
                std::swap(f, hooked_functions.back());
                hooked_functions.pop_back();

                // swap the pointers
                new_table[index] = original_table[index];

                // at this point there is no reason to swap the tables over
                // either the tables are already swapped in which case this
                // has an immediate effect, or they are not in which case its
                // not a problem anyway.

                return;
            }
        }
        assert(found);
    }

    template <typename F>
    auto get_original_function(u32 index) {
        assert(index < count_funcs());
        return reinterpret_cast<F>(original_table[index]);
    }

    auto get_instance() {
        return instance;
    }
};

template <typename T, u32 offset>
class HookFunction {
    static std::vector<HookInstance<T, offset> *> hooks;

    auto create_hook_instance(T *instance) {
        auto h = new HookInstance<T, offset>(instance);
        hooks.push_back(h);
        return h;
    }

    T * instance;
    u32 index;

public:
    HookFunction(T *instance, u32 index, void *f) {
        assert(instance);
        HookInstance<T, offset> *val = nullptr;

        for (auto &v : hooks) {
            if (v->get_instance() == instance) {
                val = v;
                break;
            }
        }

        if (val == nullptr) {
            // we havent hooked this instance yet, so do that now
            create_hook_instance(instance)->hook_function(index, f);
            return;
        }

        this->instance = instance;
        this->index    = index;

        val->hook_function(index, f);
    }

    ~HookFunction() {
        for (auto &v : hooks) {
            if (v->get_instance() == instance) {
                v->unhook_function(index);
                return;
            }
        }
    }
};

template <typename T, u32 offset>
std::vector<HookInstance<T, offset> *> HookFunction<T, offset>::hooks;

} // namespace Hooks

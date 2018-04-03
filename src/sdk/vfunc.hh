#pragma once

#include <functional>
#include <type_traits>

#include "platform.hh"
#include "types.hh"

// helpers for calling virtual functions
namespace vfunc {
inline void ** get_table(void *inst, u32 offset) {
    return *reinterpret_cast<void ***>(reinterpret_cast<u8 *>(inst) + offset);
}

inline const void ** get_table(const void *inst, u32 offset) {
    return *reinterpret_cast<const void ***>(
        reinterpret_cast<u8 *>(
            const_cast<void *>(inst)) +
        offset);
}

template <typename F>
inline auto get_func(const void *inst, u32 index, u32 offset) {
    return reinterpret_cast<F>(get_table(inst, offset)[index]);
}

template <typename F>
inline auto get_func(void *inst, u32 index, u32 offset) {
    return reinterpret_cast<F>(get_table(inst, offset)[index]);
}

template <typename>
class Func;

template <typename ObjectType, typename Ret, typename... Args>
class Func<Ret (ObjectType::*)(Args...)> {

    using FunctionType = Ret(__thiscall *)(ObjectType *, Args...);
    FunctionType f;

    // On windows this will take into account the offset
    // So we do not need to store that seperately
    ObjectType *instance;

public:
    // TODO: do any other platforms have this offset and is it useful?
    Func(ObjectType *instance,
         u32         index_windows,
         u32         index_linux,
         u32         index_osx,
         u32         offset_windows) {
        // NOTE: this assert is disabled as it is in some really tight loops!
        // This should never go off and if it does you will get a nice and obvious fatal crash...
        //assert(instance != nullptr);

        auto index = 0u;
        if constexpr (doghook_platform::windows())
            index = index_windows;
        else if constexpr (doghook_platform::linux())
            index = index_linux;
        else if constexpr (doghook_platform::osx())
            index = index_osx;

        auto offset = 0u;
        if constexpr (doghook_platform::windows()) {
            offset = offset_windows;

            this->instance = reinterpret_cast<ObjectType *>(
                reinterpret_cast<u8 *>(instance) + offset);
        } else {
            this->instance = instance;
        }

        f = get_func<FunctionType>(instance, index, offset);
    }

    Ret invoke(Args... args) {
        return f(instance, args...);
    }
};

} // namespace vfunc

// macro for easier definitions of wrapper calls
// name is the name of the function
// off is the windows offset
// varags are the arguments of the parent function
#define return_virtual_func(name, windows, linux, osx, off, ...) \
    using c = std::remove_reference<decltype(*this)>::type;      \
    using t = decltype(&c::name);                                \
    return vfunc::Func<t>(this, windows, linux, osx, off).invoke(__VA_ARGS__)

#include "precompiled.hh"

#include "gamesystem.hh"
#include "signature.hh"

static GameSystem *head = nullptr;
GameSystem::GameSystem() {
    next = head;
    head = this;
}

// TODO: remove
template <typename T>
struct UtlVector {
    T * mem;
    int alloc_count;
    int grow_size;
    int size;
    T * dbg_elements;
};

GameSystem::~GameSystem() {
    // Check whether client was already unloaded
    if (signature::resolve_library("client") == nullptr) return;

#if doghook_platform_windows()
    // We are going to have to get a little creative here...
    // Since no one except the destructors call Remove() it gets inlined into them
    // We cant call those for obvious reasons...

    // Recreate it !

    // Windows:
    // CUtlVector::FindAndRemove -> 55 8B EC 53 56 57 8B F9 33 D2 8B 77 0C
    // s_GameSystemsPerFrame     -> B9 ? ? ? ? E8 ? ? ? ? 5E 8B E5 5D C3
    // s_GameSystems             -> A1 ? ? ? ? 8B 0C B8 8B 01

    using FindAndRemoveFn   = void(__thiscall *)(void *, IGameSystem **);
    static auto find_remove = signature::find_pattern<FindAndRemoveFn>("client", "55 8B EC 53 56 57 8B F9 33 D2 8B 77 0C", 0);

    static auto s_GameSystems         = *signature::find_pattern<UtlVector<IGameSystem *> **>("client", "A1 ? ? ? ? 8B 0C B8 8B 01", 1);
    static auto s_GameSystemsPerFrame = *signature::find_pattern<UtlVector<IGameSystem *> **>("client", "A1 ? ? ? ? 8B 35 ? ? ? ? 8B CE 8B 04 B8", 1);

    auto to_find = (IGameSystem *)this;

    IGameSystem **mem = s_GameSystems->mem;

    find_remove(s_GameSystems, &to_find);
    find_remove(s_GameSystemsPerFrame, &to_find);
#else

#endif
}

void GameSystem::add_all() {
    using AddFn         = void (*)(IGameSystem *);
    static AddFn add_fn = []() {
        if constexpr (doghook_platform::windows()) {
            return reinterpret_cast<AddFn>(
                signature::resolve_callgate(
                    signature::find_pattern("client", "E8 ? ? ? ? 83 C4 04 8B 76 04 85 F6 75 D0")));
        } else if constexpr (doghook_platform::linux()) {
            return reinterpret_cast<AddFn>(
                signature::resolve_callgate(
                    signature::find_pattern("client", "E8 ? ? ? ? 8B 5B 04 85 DB 75 C1")));
        } else if constexpr (doghook_platform::osx()) {
            return reinterpret_cast<AddFn>(
                signature::resolve_callgate(
                    signature::find_pattern("client", "E8 ? ? ? ? 8B 7F 04 85 FF 75 A1")));
        }
    }();

    assert(add_fn);

    for (auto system = head; system != nullptr; system = system->next) {
        // Call original Add() function
        ((void (*)(IGameSystem *))add_fn)(system);
        system->init();
    }

    for (auto system = head; system != nullptr; system = system->next) {
        system->post_init();
    }

    // reset the list
    head = nullptr;
}

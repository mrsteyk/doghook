#pragma once

// These classes must remain by these names in order to fufill some rtti hacks when adding gamesystems
// DO NOT CHANGE THEM

class IGameSystem {
public:
    // GameSystems are expected to implement these methods.
    virtual char const *name() = 0;

    // Init, shutdown
    // return true on success. false to abort DLL init!
    virtual bool init()      = 0;
    virtual void post_init() = 0;
    virtual void shutdown()  = 0;

    // Level init, shutdown
    virtual void level_init_pre_entity() = 0;
    // entities are created / spawned / precached here
    virtual void level_init_post_entity() = 0;

    virtual void level_shutdown_pre_clear_steam_api_context() = 0;
    virtual void level_shutdown_pre_entity()                  = 0;
    // Entities are deleted / released here...
    virtual void level_shutdown_post_entity() = 0;
    // end of level shutdown

    // Called during game save
    virtual void on_save() = 0;

    // Called during game restore, after the local player has connected and entities have been fully restored
    virtual void on_restore() = 0;

    // Called every frame. It's safe to remove an igamesystem from within this callback.
    virtual void safe_remove_if_desired() = 0;

    virtual bool is_per_frame() = 0;

    // destructor, cleans up automagically....
    virtual ~IGameSystem(){};

    // Client systems can use this to get at the map name
    static const char *map_name();

    // These methods are used to add and remove server systems from the
    // main server loop. The systems are invoked in the order in which
    // they are added.
private:
    static void add(IGameSystem *pSys);
    static void remove(IGameSystem *pSys);
    static void remove_all();

    // These methods are used to initialize, shutdown, etc all systems
    static bool init_all_systems();
    static void post_init_all_systems();
    static void shutdown_all_systems();
    static void level_init_pre_entity_all_systems(char const *pMapName);
    static void level_init_post_all_systems();
    static void level_shutdown_pre_clear_steam_api_context_all_systems(); // Called prior to steamgameserverapicontext->Clear()
    static void level_shutdown_pre_entity_all_systems();
    static void level_shutdown_post_entity_all_systems();

    static void on_save_all_systems();
    static void on_restore_all_systems();

    static void safe_remove_if_desired_all_systems();

    static void pre_render_all_systems();
    static void update_all_systems(float frametime);
    static void post_render_all_systems();
};

class IGameSystemPerFrame : public IGameSystem {
public:
    // destructor, cleans up automagically....
    virtual ~IGameSystemPerFrame(){};

    // Called before rendering
    virtual void pre_render() = 0;

    // Gets called each frame
    virtual void update(float frametime) = 0;

    // Called after rendering
    virtual void post_render() = 0;
};

// Quick and dirty server system for users who don't care about precise ordering
// and usually only want to implement a few of the callbacks
class CBaseGameSystemPerFrame : public IGameSystemPerFrame {

public:
    virtual const char *name() override { return "unnamed"; }

    // Init, shutdown
    // return true on success. false to abort DLL init!
    virtual bool init() override { return true; }
    virtual void post_init() override {}
    virtual void shutdown() override {}

    // Level init, shutdown
    virtual void level_init_pre_entity() override {}
    virtual void level_init_post_entity() override {}
    virtual void level_shutdown_pre_clear_steam_api_context() override {}
    virtual void level_shutdown_pre_entity() override {}
    virtual void level_shutdown_post_entity() override {}

    virtual void on_save() override {}
    virtual void on_restore() override {}
    virtual void safe_remove_if_desired() override {}

    virtual bool is_per_frame() override { return true; }

    // Called before rendering
    virtual void pre_render() override {}

    // Gets called each frame
    virtual void update(float) override {}

    // Called after rendering
    virtual void post_render() override {}
};

// TODO: GameSystem could be in a namespace (not directly required for the rtti hack)...

class GameSystem : public CBaseGameSystemPerFrame {
    GameSystem *next;

public:
    GameSystem();
    virtual ~GameSystem();

    // add all functions that are in the linked list
    static void add_all();
};

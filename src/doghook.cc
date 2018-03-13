#include "stdafx.hh"

#include "gamesystem.hh"
#include "log.hh"

#include "class_id.hh"
#include "hooks.hh"
#include "interface.hh"
#include "netvar.hh"
#include "player.hh"
#include "sdk.hh"
#include "vfunc.hh"

// Singleton for doing init / deinit of doghook
// and dealing with hooks from gamesystem

class Doghook : public GameSystem {
    bool inited = false;

public:
    bool init() override {
        // Guard against having init() called by the game and our constructor
        static bool init_happened = false;
        if (init_happened) return true;

        Log::msg("init()");
        init_happened = true;
        return true;
    }

    void post_init() override {
        Log::msg("post_init()");

        // Get interfaces here before init_all has a chance to do anything

        IFace<sdk::Client>().set_from_interface("client", "VClient");
        IFace<sdk::Engine>().set_from_interface("engine", "VEngineClient");
        IFace<sdk::EntList>().set_from_interface("client", "VClientEntityList");
        IFace<sdk::Input>().set_from_pointer(**reinterpret_cast<sdk::Input ***>(
            vfunc::get_func<u8 *>(IFace<sdk::Client>().get(), 15, 0) + 0x2));
        IFace<sdk::Cvar>().set_from_interface("vstdlib", "VEngineCvar");

        // TODO linux signatures
        IFace<sdk::ClientMode>().set_from_pointer(
            *signature::find_pattern<sdk::ClientMode **>(
                "client", "B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C7 05", 1));
        IFace<sdk::ModelInfo>().set_from_interface("engine", "VModelInfoClient");
        IFace<sdk::Trace>().set_from_interface("engine", "EngineTraceClient");
        IFace<sdk::DebugOverlay>().set_from_interface("engine", "VDebugOverlay");
        IFace<sdk::PlayerInfoManager>().set_from_interface("server", "PlayerInfoManager");

        IFace<sdk::Globals>().set_from_pointer(IFace<sdk::PlayerInfoManager>()->globals());

        auto globals_server_address = (u32)IFace<sdk::Globals>().get();

        // TODO: this globals_real_address is windows only!
        if constexpr (doghook_platform::windows()) {
            auto globals_real_address = (u32)*signature::find_pattern<sdk::Globals **>("engine", "A1 ? ? ? ? 8B 11 68", 8);
            IFace<sdk::Globals>().set_from_pointer((sdk::Globals *)globals_real_address);
        }

        IFace<sdk::GameMovement>().set_from_interface("client", "GameMovement");
        IFace<sdk::Prediction>().set_from_interface("client", "VClientPrediction");
        IFace<sdk::MoveHelper>().set_from_pointer(
            *signature::find_pattern<sdk::MoveHelper **>(
                "client", "8B 0D ? ? ? ? 8B 01 FF 50 28 56", 2));
    }

    void process_attach() {
        Log::msg("process_attach()");

        // TODO: should these two be modules??

        // make sure that the netvars are initialised
        // becuase their dynamic initialiser could be after the
        // gamesystems one
        sdk::Netvar::init_all();

        // TODO:

        // register all convars now that we have the interfaces we need
        //Convar_Base::init_all();

        // at this point we are now inited and ready to go!
        inited = true;
    }

    void shutdown() override {}

    void level_init_pre_entity() override { Log::msg("init_pre_entity()"); }
    void level_init_post_entity() override {
        Log::msg("level_init_post_entity");

        // Make sure that all our class_ids are correct
        // This will only do anything on debug builds and not on release builds.

        // TODO:

        // This needs to be done here becuase classids arent initialised before we are in game
        sdk::class_id::internal_checker::ClassIDChecker::check_all_correct();
    }
    auto level_shutdown_pre_clear_steam_api_context() -> void override { Log::msg("level_shutdown_pre_clear_steam_api_context"); }
    auto level_shutdown_pre_entity() -> void override {
        Log::msg("level_shutdown_pre_entity");
    }
    auto level_shutdown_post_entity() -> void override { Log::msg("level_shutdown_post_entity"); }

    // update is called from CHLClient_HudUpdate
    // in theory we should be able to render here
    // and be perfectly ok
    // HOWEVER: it might be better to do this at frame_end()
    void update([[maybe_unused]] float frametime) override {
        if (inited != true || IFace<sdk::Engine>()->in_game() != true) return;
    }

    Doghook() {
        GameSystem::add_all();
    }
};

Doghook doghook;

#if doghook_platform_windows()
auto __stdcall doghook_process_attach([[maybe_unused]] void *hmodule) -> u32 {
    // TODO: pass module over to the gamesystem
    doghook.process_attach();

    return 0;
}
#endif

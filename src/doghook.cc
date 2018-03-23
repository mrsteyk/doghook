#include "precompiled.hh"

#include "doghook.hh"

#include "sdk/gamesystem.hh"
#include "sdk/log.hh"

#include "sdk/class_id.hh"
#include "sdk/convar.hh"
#include "sdk/draw.hh"
#include "sdk/hooks.hh"
#include "sdk/interface.hh"
#include "sdk/netvar.hh"
#include "sdk/player.hh"
#include "sdk/sdk.hh"
#include "sdk/vfunc.hh"

#include "hooks/createmove.hh"
#include "hooks/engine_vgui.hh"

#include "sdk/signature.hh"

// Singleton for doing init / deinit of doghook
// and dealing with hooks from gamesystem

class Doghook : public GameSystem {
    bool inited = false;

public:
    bool init() override {
        // Guard against having init() called by the game and our constructor
        static bool init_happened = false;
        if (init_happened) return true;

        logging::msg("init()");
        init_happened = true;
        return true;
    }

    void post_init() override {
        logging::msg("post_init()");

        // Get interfaces here before init_all has a chance to do anything

        IFace<sdk::Client>().set_from_interface("client", "VClient");
        IFace<sdk::Engine>().set_from_interface("engine", "VEngineClient");
        IFace<sdk::EntList>().set_from_interface("client", "VClientEntityList");

        if constexpr (doghook_platform::windows())
            IFace<sdk::Input>().set_from_pointer(**reinterpret_cast<sdk::Input ***>(
                vfunc::get_func<u8 *>(IFace<sdk::Client>().get(), 15, 0) + 0x2));
        else if constexpr (doghook_platform::linux())
            IFace<sdk::Input>().set_from_pointer(**reinterpret_cast<sdk::Input ***>(
                vfunc::get_func<u8 *>(IFace<sdk::Client>().get(), 15, 0) + 0x1));

        IFace<sdk::Cvar>().set_from_interface("vstdlib", "VEngineCvar");

        if constexpr (doghook_platform::windows())
            IFace<sdk::ClientMode>().set_from_pointer(
                *signature::find_pattern<sdk::ClientMode **>(
                    "client", "B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C7 05", 1));
        else if constexpr (doghook_platform::linux()) {
            // ClientMode is a magic static. So getting a sig for it is difficult (conflicts with all other magic statics)
            // So we are going to do some multistage shit in order to retrieve it
            auto outer_function = signature::find_pattern<void *>("client", "55 89 E5 83 EC 18 E8 ? ? ? ? A3 ? ? ? ? E8", 6);
            assert(outer_function);

            auto inner_function = static_cast<u8 *>(signature::resolve_callgate(outer_function));
            assert(outer_function);

            IFace<sdk::ClientMode>().set_from_pointer(*reinterpret_cast<sdk::ClientMode **>(inner_function + 10));
            assert(IFace<sdk::ClientMode>().get());
        }

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
        } else if constexpr (doghook_platform::linux()) {
            auto globals_real_address = (u32) * *signature::find_pattern<sdk::Globals ***>("client", "8B 15 ? ? ? ? F3 0F 10 88 D0 08 00 00", 2);

            IFace<sdk::Globals>().set_from_pointer((sdk::Globals *)globals_real_address);
        }

        IFace<sdk::GameMovement>().set_from_interface("client", "GameMovement");
        IFace<sdk::Prediction>().set_from_interface("client", "VClientPrediction");

        if constexpr (doghook_platform::windows())
            IFace<sdk::MoveHelper>().set_from_pointer(
                *signature::find_pattern<sdk::MoveHelper **>(
                    "client", "8B 0D ? ? ? ? 8B 01 FF 50 28 56", 2));
        else if constexpr (doghook_platform::linux())
            IFace<sdk::MoveHelper>().set_from_pointer(nullptr);

        inited = true;
    }

    void process_attach() {
        // Wait for init...
        while (!inited) {
            std::this_thread::yield();
        }

        logging::msg("process_attach()");

        // make sure that the netvars are initialised
        // becuase their dynamic initialiser could be after the
        // gamesystems one
        sdk::Netvar::init_all();

        // register all convars now that we have the interfaces we need
        ConvarBase::init_all();

        // Setup drawing and paint hook
        sdk::draw::init(sdk::draw::RenderTarget::surface);
        paint_hook::init_all();

        // at this point we are now inited and ready to go!
        inited = true;
    }

    void shutdown() override {}

    void level_init_pre_entity() override { logging::msg("init_pre_entity()"); }
    void level_init_post_entity() override {
        logging::msg("level_init_post_entity");

        // Make sure that all our class_ids are correct
        // This will only do anything on debug builds and not on release builds.

        // This needs to be done here becuase classids arent initialised before we are in game
        sdk::class_id::internal_checker::ClassIDChecker::check_all_correct();

        // Do level init here
        create_move::level_init();
    }
    auto level_shutdown_pre_clear_steam_api_context() -> void override { logging::msg("level_shutdown_pre_clear_steam_api_context"); }
    auto level_shutdown_pre_entity() -> void override {
        logging::msg("level_shutdown_pre_entity");

        // Do level_shutdown here
        create_move::level_shutdown();
    }
    auto level_shutdown_post_entity() -> void override { logging::msg("level_shutdown_post_entity"); }

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
#elif doghook_platform_linux()
void doghook_process_attach() {
    doghook.process_attach();
}
#endif

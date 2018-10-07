#include "precompiled.hh"

#include "doghook.hh"

#include "sdk/class_id.hh"
#include "sdk/convar.hh"
#include "sdk/draw.hh"
#include "sdk/gamesystem.hh"
#include "sdk/hooks.hh"
#include "sdk/interface.hh"
#include "sdk/log.hh"
#include "sdk/netvar.hh"
#include "sdk/player.hh"
#include "sdk/sdk.hh"
#include "sdk/signature.hh"
#include "sdk/vfunc.hh"

#include "hooks/createmove.hh"
#include "hooks/engine_vgui.hh"
#include "hooks/send_datagram.hh"

#include "modules/esp.hh"
#include "modules/menu.hh"
#include "modules/misc.hh"

#include "utils/profiler.hh"

sdk::Convar<bool>        doghook_profiling_enabled{"doghook_profiling_enabled", false, nullptr};
static sdk::Convar<bool> doghook_unload_now{"doghook_unload_now", false, nullptr};

// Singleton for doing init / deinit of doghook
// and dealing with hooks from gamesystem

extern class Doghook doghook;

class Doghook : public GameSystem {
public:
    bool inited       = false;
    bool shutdown_now = false;

#if doghook_platform_windows()
    HMODULE doghook_module_handle;
#endif

    static void await_shutdown() {
        while (!doghook_unload_now && !doghook.shutdown_now) {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(1000ms);
            std::this_thread::yield();
        }

        // Not a user called unload - the process is going down anyway so just
        // Let that happen
        if (doghook.shutdown_now && !doghook_unload_now) return;

#if doghook_platform_windows()
        // TODO: the CRT should be able to take care of all our hooks as they are all declared
        // as unique_ptrs...

        // All other memory should be cleaned up in destructors that happen at init_time or deinit_time!!
        // TODO: make sure that all memory is getting cleaned properly!
        FreeLibrary(doghook.doghook_module_handle);
#endif
    }

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

        IFace<sdk::Client>::set_from_interface("client", "VClient");
        IFace<sdk::Engine>::set_from_interface("engine", "VEngineClient");
        IFace<sdk::EntList>::set_from_interface("client", "VClientEntityList");
        IFace<sdk::Cvar>::set_from_interface("vstdlib", "VEngineCvar");
        IFace<sdk::ModelInfo>::set_from_interface("engine", "VModelInfoClient");
        IFace<sdk::Trace>::set_from_interface("engine", "EngineTraceClient");
        IFace<sdk::DebugOverlay>::set_from_interface("engine", "VDebugOverlay");
        IFace<sdk::GameMovement>::set_from_interface("client", "GameMovement");
        IFace<sdk::Prediction>::set_from_interface("client", "VClientPrediction");
        IFace<sdk::InputSystem>::set_from_interface("inputsystem", "InputSystemVersion");

        if constexpr (doghook_platform::windows()) {
            IFace<sdk::Input>::set_from_pointer(**reinterpret_cast<sdk::Input ***>(
                vfunc::get_func<u8 *>(sdk::iface::client, 15, 0) + 0x2));

            IFace<sdk::ClientMode>::set_from_pointer(
                *signature::find_pattern<sdk::ClientMode **>(
                    "client", "B9 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 C7 05", 1));

            auto globals_real_address = (u32)*signature::find_pattern<sdk::Globals **>("engine", "A1 ? ? ? ? 8B 11 68", 8);
            IFace<sdk::Globals>::set_from_pointer((sdk::Globals *)globals_real_address);

            IFace<sdk::MoveHelper>::set_from_pointer(
                *signature::find_pattern<sdk::MoveHelper **>(
                    "client", "8B 0D ? ? ? ? 8B 01 FF 50 28 56", 2));

        } else if constexpr (doghook_platform::linux()) {
            IFace<sdk::Input>::set_from_pointer(**reinterpret_cast<sdk::Input ***>(
                vfunc::get_func<u8 *>(sdk::iface::client, 15, 0) + 0x1));

            // ClientMode is a magic static. So getting a sig for it is difficult (conflicts with all other magic statics)
            // So we are going to do some multistage shit in order to retrieve it
            auto outer_function = signature::find_pattern<void *>("client", "55 89 E5 83 EC 18 E8 ? ? ? ? A3 ? ? ? ? E8", 6);
            assert(outer_function);

            auto inner_function = static_cast<u8 *>(signature::resolve_callgate(outer_function));
            assert(outer_function);

            IFace<sdk::ClientMode>::set_from_pointer(*reinterpret_cast<sdk::ClientMode **>(inner_function + 10));
            assert(sdk::iface::client_mode);

            auto globals_real_address = (u32) * *signature::find_pattern<sdk::Globals ***>("client", "8B 15 ? ? ? ? F3 0F 10 88 D0 08 00 00", 2);

            IFace<sdk::Globals>::set_from_pointer((sdk::Globals *)globals_real_address);

            IFace<sdk::MoveHelper>::set_from_pointer(nullptr);
        }

#if 0
        IFace<sdk::PlayerInfoManager>::set_from_interface("server", "PlayerInfoManager");
        iface::sdk::Globals>::set_from_pointer(IFace<sdk::PlayerInfoManager->globals());
        auto globals_server_address = (u32)iface::sdk::Globals.get();
#endif

        inited = true;
    }

    void process_attach() {
        // Wait for init...
        while (!inited) {
            std::this_thread::yield();
        }

        logging::msg("process_attach()");

        // make sure that the profiler is inited first
        profiler::init();

        // make sure that the netvars are initialised
        // becuase their dynamic initialiser could be after the
        // gamesystems one
        sdk::Netvar::init_all();

        // register all convars now that we have the interfaces we need
        sdk::ConvarBase::init_all();

        // Setup drawing and paint hook
        // sdk::draw::init(sdk::draw::RenderTarget::overlay);
        paint_hook::init_all();

        misc::init_all();

        logging::msg("DOGHOOK:tm: :joy: :joy: :jo3: :nice:\nBuild: " __DATE__ " " __TIME__);

        menu::init();

        // at this point we are now inited and ready to go!
        inited = true;

        std::thread{&await_shutdown}.detach();

        // If we are already in game then do the level inits
        if (sdk::iface::engine->in_game()) {
            level_init_pre_entity();
            level_init_post_entity();
        }
    }

    void shutdown() override {
        shutdown_now = true;
        paint_hook::shutdown_all();
    }

    void level_init_pre_entity() override { logging::msg("init_pre_entity()"); }
    void level_init_post_entity() override {
        logging::msg("level_init_post_entity");

        // Make sure that all our class_ids are correct
        // This will only do anything on debug builds and not on release builds.

        // This needs to be done here becuase classids arent initialised before we are in game
        sdk::class_id::internal_checker::ClassIDChecker::check_all_correct();

        // Do level init here
        create_move::level_init();
        send_datagram::level_init();
    }
    void level_shutdown_pre_clear_steam_api_context() override { logging::msg("level_shutdown_pre_clear_steam_api_context"); }
    void level_shutdown_pre_entity() override {
        logging::msg("level_shutdown_pre_entity");

        // Do level_shutdown here
        create_move::level_shutdown();
        send_datagram::level_shutdown();
    }
    void level_shutdown_post_entity() override { logging::msg("level_shutdown_post_entity"); }

    // update is called from CHLClient_HudUpdate
    // in theory we should be able to render here
    // and be perfectly ok
    // HOWEVER: it might be better to do this at frame_end()
    void update(float frametime) override {
        if (inited != true) return;
        profiler::set_profiling_enabled(doghook_profiling_enabled);

        misc::update(frametime);
    }

    Doghook() {
        GameSystem::add_all();
    }
};

Doghook doghook;

#if doghook_platform_windows()
u32 __stdcall doghook_process_attach(void *hmodule) {
    // TODO: pass module over to the gamesystem
    doghook.process_attach();

    doghook.doghook_module_handle = (HMODULE)hmodule;

    return 0;
}
#elif doghook_platform_linux()
void doghook_process_attach() {
    doghook.process_attach();
}
#endif

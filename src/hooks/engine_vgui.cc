#include <precompiled.hh>

#include <sdk/draw.hh>
#include <sdk/hooks.hh>
#include <sdk/log.hh>
#include <sdk/sdk.hh>

using namespace sdk;

namespace paint_hook {
hooks::HookFunction<EngineVgui, 0> *engine_vgui_hook;

using StartDrawing  = void(__thiscall *)(void *);
using FinishDrawing = void(__thiscall *)(void *);

void hooked_paint(EngineVgui *instance, u32 paint_method) {
    engine_vgui_hook->call_original<void>(paint_method);

    if (paint_method & 1) {
        draw::start();

        // Draw here!

        draw::finish();
    }
}

void init_all() {
    // hook up
    engine_vgui_hook = new hooks::HookFunction<EngineVgui, 0>(IFace<EngineVgui>().get(), 14, 14, 14, reinterpret_cast<void *>(&hooked_paint));
}

void shutdown_all() {
    delete engine_vgui_hook;
}

} // namespace paint_hook

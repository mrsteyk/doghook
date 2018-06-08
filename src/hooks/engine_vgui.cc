#include <precompiled.hh>

#include <sdk/draw.hh>
#include <sdk/hooks.hh>
#include <sdk/log.hh>
#include <sdk/platform.hh>
#include <sdk/sdk.hh>

#include "modules/esp.hh"

using namespace sdk;

namespace paint_hook {
std::unique_ptr<hooks::HookFunction<EngineVgui, 0>> engine_vgui_hook;

using StartDrawing  = void(__thiscall *)(void *);
using FinishDrawing = void(__thiscall *)(void *);

#if doghook_platform_windows()
void __fastcall hooked_paint(EngineVgui *instance, void *edx, u32 paint_method)
#else
void hooked_paint(EngineVgui *instance, u32 paint_method)
#endif
{
    engine_vgui_hook->call_original<void>(paint_method);

    if (paint_method & 1) {
        draw::start();

        // Draw here!
        esp::paint();

        draw::finish();
    }
}

void init_all() {
    // hook up
    engine_vgui_hook = std::make_unique<hooks::HookFunction<EngineVgui, 0>>(iface::engine_vgui, 13, 14, 14, reinterpret_cast<void *>(&hooked_paint));
}

void shutdown_all() {
    engine_vgui_hook.reset();
}

} // namespace paint_hook

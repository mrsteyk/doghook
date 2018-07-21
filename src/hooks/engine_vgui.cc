#include <precompiled.hh>

#include <sdk/convar.hh>
#include <sdk/draw.hh>
#include <sdk/hooks.hh>
#include <sdk/log.hh>
#include <sdk/platform.hh>
#include <sdk/sdk.hh>

#include "modules/esp.hh"
#include "modules/menu.hh"

#include "utils/profiler.hh"
#include "utils/semaphore.hh"

#include "imgui.h"

using namespace sdk;

namespace paint_hook {
std::unique_ptr<hooks::HookFunction<EngineVgui, 0>> engine_vgui_hook;

using StartDrawing  = void(__thiscall *)(void *);
using FinishDrawing = void(__thiscall *)(void *);

bool shutdown_all_triggered = false;

Convar<bool> paint_swap_disabled{"doghook_paint_swap_disabled", false, nullptr};

Semaphore overlay_begin_draw;
Semaphore overlay_finished_draw;

void init_paint_hook();

void paint() {
    // Draw here!
    esp::paint();
    menu::paint();
}

float engine_vgui_begin_time;
float engine_vgui_draw_time;
float engine_vgui_finish_time;

void paint_thread() {
    draw::init(draw::RenderTarget::overlay);

    paint_hook::init_paint_hook();

    if (!draw::overlay()) {
        return;
    }

    while (true) {
        overlay_begin_draw.wait();

        if (shutdown_all_triggered) {
            return;
        }

        // TODO: fix the funkyness associated with using the overlay

        // On windows:
        // using the overlay causes the steam overlay to "disappear" -
        // it wont open when shift-tab is pressed.
        // Removing the call to SwapBuffers(dc) in oxide_windows fixes this problem
        // - so this is not caused by the overlay itself, but either when we get it to
        // swap or when it decides to swap.

        // On linux:
        // Opening the steam overlay causes even more problems which can then propogate into
        // the game itself (causing broken lighting, ...)
        // This is caused by our call to glXSwapBuffers in oxide - and disabling it solves the issue

        draw::swap(!paint_swap_disabled);

        profiler::Timer t;

        t.start();
        draw::start();
        t.end();

        engine_vgui_begin_time = t.milliseconds();

        t.start();
        paint();
        t.end();
        engine_vgui_draw_time = t.milliseconds();

        // run finish after notifty so that we dont stop the game from rendering frames
        overlay_finished_draw.notify();

        t.start();
        draw::finish();
        t.end();
        engine_vgui_finish_time = t.milliseconds();
    }
}

void notify_begin_draw() {
    if (draw::overlay()) overlay_begin_draw.notify();
}
void wait_for_end_draw() {
    if (draw::overlay()) overlay_finished_draw.wait();
}

#if doghook_platform_windows()
void __fastcall hooked_paint(EngineVgui *instance, void *edx, u32 paint_method)
#else
void hooked_paint(EngineVgui *instance, u32 paint_method)
#endif
{
    engine_vgui_hook->call_original<void>(paint_method);

    if (paint_method & 1) {
        if (draw::surface()) {
            draw::start();
            paint();
            draw::finish();
        } else {
            notify_begin_draw();
            wait_for_end_draw();
        }
    }
}

void init_all() {
    //draw::init(draw::RenderTarget::surface);
    std::thread{&paint_thread}.detach();
}

void init_paint_hook() {
    // set up surface interfaces
    iface::engine_vgui.set_from_interface("engine", "VEngineVGui");

    engine_vgui_hook = std::make_unique<hooks::HookFunction<EngineVgui, 0>>(iface::engine_vgui, 13, 14, 14, reinterpret_cast<void *>(&hooked_paint));
}

void shutdown_all() {
    engine_vgui_hook.reset();

    // Shutdown thread
    shutdown_all_triggered = true;
    notify_begin_draw();
}

} // namespace paint_hook

#include <precompiled.hh>

#include <cstdarg>
#include <cstdio>

#include "draw.hh"

#include "hooks.hh"
#include "interface.hh"
#include "log.hh"
#include "sdk.hh"
#include "vfunc.hh"

#include "utils/profiler.hh"

namespace sdk::draw {

class Surface {
public:
    using FontHandle = u32;

    void set_color(int r, int g, int b, int a) {
        return_virtual_func(set_color, 11, 10, 10, 0, r, g, b, a);
    }

    void filled_rect(int x0, int y0, int x1, int y1) {
        return_virtual_func(filled_rect, 12, 12, 12, 0, x0, y0, x1, y1);
    }

    void outlined_rect(int x0, int y0, int x1, int y1) {
        return_virtual_func(outlined_rect, 14, 14, 14, 0, x0, y0, x1, y1);
    }

    void set_font_handle(FontHandle f) {
        return_virtual_func(set_font_handle, 17, 17, 17, 0, f);
    }

    void set_font_color(int r, int g, int b, int a) {
        return_virtual_func(set_font_color, 19, 18, 18, 0, r, g, b, a);
    }

    void set_text_pos(int x, int y) {
        return_virtual_func(set_text_pos, 20, 20, 20, 0, x, y);
    }

    void text(const wchar_t *text, u32 len, int unk1 = 1) {
        return_virtual_func(text, 22, 22, 22, 0, text, len, unk1);
    }

    FontHandle create_font() {
        return_virtual_func(create_font, 66, 66, 66, 0);
    }

    void set_font_attributes(FontHandle font, const char *font_name,
                             int tall, int weight, int blur, int scanlines, int flags, int unk1 = 0, int unk2 = 0) {
        return_virtual_func(set_font_attributes, 67, 67, 67, 0, font, font_name, tall, weight, blur, scanlines, flags, unk1, unk2);
    }

    void text_size(FontHandle f, const char *text, int &wide, int &tall) {
        return_virtual_func(text_size, 75, 75, 75, 0, f, text, wide, tall);
    }
};

RenderTarget current_render_target;
auto         is_surface() { return current_render_target == RenderTarget::surface; }
auto         is_overlay() { return current_render_target == RenderTarget::overlay; }

void init(RenderTarget t) {
    if (t == RenderTarget::overlay) {
    } else if (t == RenderTarget::surface) {
#if doghook_platform_linux()
        std::setlocale(LC_ALL, "en_US.utf8");
#endif

        // set up surface interfaces
        iface::engine_vgui.set_from_interface("engine", "VEngineVGui");
        IFace<Surface>().set_from_interface("vguimatsurface", "VGUI_Surface030");

        current_render_target = RenderTarget::surface;
    }
}

void start() {
    assert(is_surface());
    using StartDrawing = void(__thiscall *)(Surface *);

    // look for -pixel_offset_x

    static StartDrawing start_drawing = []() -> auto {
        if constexpr (doghook_platform::linux()) {
            return signature::find_pattern<StartDrawing>("vguimatsurface", "55 89 E5 53 81 EC 94 00 00 00", 0);
        } else if constexpr (doghook_platform::windows()) {
            return signature::find_pattern<StartDrawing>("vguimatsurface", "55 8B EC 64 A1 ? ? ? ? 6A FF 68 ? ? ? ? 50 64 89 25 ? ? ? ? 83 EC 14", 0);
        }
    }
    ();

    start_drawing(IFace<Surface>().get());
}

void finish() {
    assert(is_surface());

    using FinishDrawing = void(__thiscall *)(Surface *);

    static FinishDrawing finish_drawing = []() -> auto {
        if constexpr (doghook_platform::linux()) {
            return signature::find_pattern<FinishDrawing>("vguimatsurface", "55 89 E5 53 83 EC 24 C7 04 24 00 00 00 00", 0);
        } else if constexpr (doghook_platform::windows()) {
            return signature::find_pattern<FinishDrawing>("vguimatsurface", "55 8B EC 6A FF 68 ? ? ? ? 64 A1 ? ? ? ? 50 64 89 25 ? ? ? ? 51 56 6A 00", 0);
        }
    }
    ();

    finish_drawing(IFace<Surface>().get());
}

bool world_to_screen(const math::Vector &world, math::Vector &screen) {
    if (is_surface()) {
        return !iface::overlay->screen_position(world, screen);
    }

    return false;
}

void set_color(Color c) {
    if (is_surface()) {
        IFace<Surface>()->set_color(c.r, c.g, c.b, c.a);
    }
}

void filled_rect(Color c, math::Vector p1, math::Vector p2) {
    if (is_surface()) {
        set_color(c);

        IFace<Surface>()->filled_rect(p1.x, p1.y, p2.x, p2.y);
    }
}

void outlined_rect(Color c, math::Vector p1, math::Vector p2) {
    if (is_surface()) {
        set_color(c);

        IFace<Surface>()->outlined_rect(p1.x, p1.y, p2.x, p2.y);
    }
}

FontHandle register_font(const char *font_name, u32 size) {
    if (is_surface()) {
        auto h = IFace<Surface>()->create_font();
        IFace<Surface>()->set_font_attributes(h, font_name, size, 500, 0, 0, 0x010 | 0x080);

        return h;
    }

    return -1;
}

#include <cstdarg>

void text(FontHandle h, Color c, math::Vector p, const char *format, ...) {
    profiler_profile_function();
    if (is_surface()) {

        char    buffer[1024];
        wchar_t buffer_wide[1024];
        memset(buffer, 0, sizeof(buffer));

        va_list args;
        va_start(args, format);
        vsnprintf(buffer, 1023, format, args);
        va_end(args);

        // TODO: is this even necessary??
        const wchar_t *format;
        if constexpr (doghook_platform::windows())
            format = L"%S";
        else if constexpr (doghook_platform::linux())
            format = L"%s";

        swprintf(buffer_wide, 1024, format, buffer);

        IFace<Surface>()->set_font_handle(h);
        IFace<Surface>()->set_font_color(c.r, c.g, c.b, c.a);
        IFace<Surface>()->set_text_pos(p.x, p.y);

        IFace<Surface>()->text(buffer_wide, wcslen(buffer_wide));
    }
}

} // namespace sdk::draw

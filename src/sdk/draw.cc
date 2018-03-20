#include <precompiled.hh>

#include "draw.hh"

#include "hooks.hh"
#include "interface.hh"
#include "sdk.hh"
#include "vfunc.hh"

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
        return_virtual_func(set_font_color, 19, 18, 18, 0, r, b, g, a);
    }

    void set_text_pos(int x, int y) {
        return_virtual_func(set_text_pos, 20, 20, 20, 0, x, y);
    }

    // TODO: On windows this really should be wchar_t
    void text(const char *text, u32 len) {
        return_virtual_func(text, 22, 22, 22, 0, text, len);
    }

    FontHandle create_font() {
        return_virtual_func(create_font, 66, 66, 66, 0);
    }

    void set_font_attributes(FontHandle &font, const char *font_name,
                             int tall, int weight, int blur, int scanlines, int flags) {
        return_virtual_func(set_font_attributes, 67, 67, 67, 0, font, font_name, tall, weight, blur, scanlines, flags);
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
        // set up surface interfaces
        IFace<EngineVgui>().set_from_interface("engine", "VEngineVGui");
        IFace<Surface>().set_from_interface("vguimatsurface", "VGUI_Surface030");
    }
}

void start() {
    assert(is_surface());
    using StartDrawing = void(__thiscall *)(Surface *);

    static StartDrawing start_drawing = []() -> auto {
        if constexpr (doghook_platform::linux()) {
            return signature::find_pattern<StartDrawing>("vguimatsurface", "55 89 E5 53 81 EC 94 00 00 00", 0);
        } else if constexpr (doghook_platform::windows()) {
            return nullptr;
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
            return nullptr;
        }
    }
    ();

    finish_drawing(IFace<Surface>().get());
}

math::Vector world_to_screen(math::Vector &v) {
    math::Vector result;
    auto         is_visible = IFace<DebugOverlay>()->screen_position(v, result);

    if (is_visible) return result;

    return math::Vector::invalid();
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

} // namespace sdk::draw

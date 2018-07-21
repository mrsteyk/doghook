#pragma once

#include "sdk/platform.hh"

// Drawing front end for doghook

namespace sdk {
namespace draw {

class Color {
public:
    u8 r, g, b, a;

    Color()               = default;
    Color(const Color &c) = default;
    explicit Color(u32 c) {
        auto array = (u8 *)&c;
        r          = array[3];
        g          = array[2];
        b          = array[1];
        a          = array[0];
    }
    Color(u8 r, u8 g, u8 b, u8 a) : r(r), g(g), b(b), a(a) {
    }
};

enum class RenderTarget {
    surface,
    overlay
};

// initialise the drawing state
void init(RenderTarget t);

RenderTarget current_target();

inline auto surface() { return current_target() == RenderTarget::surface; }
inline auto overlay() { return current_target() == RenderTarget::overlay; }

// Sets whether the buffer swap is enabled (for testing)
void swap(bool enabled);

// suface start drawing and finish drawing functions
void start();
void finish();

// returns the screen coords of the world space point world in screen
bool world_to_screen(const math::Vector &world, math::Vector &screen);

// TODO: drawing primitives + setting colors

// draw a filled rectangle from p1 to p2
void filled_rect(Color c, math::Vector p1, math::Vector p2);

// draw an outlined rectangle from p1 to p2
void outlined_rect(Color c, math::Vector p1, math::Vector p2);

void line(Color c, math::Vector p1, math::Vector p2);

using FontHandle = u32;

// Register a font
FontHandle register_font(const char *file_name, const char *font_name, u32 size);

void text(FontHandle f, u32 size, Color c, math::Vector p, const char *format, ...);

// TODO: drawing fonts + setting colors

} // namespace draw
} // namespace sdk

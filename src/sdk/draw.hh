#pragma once

#include "sdk/platform.hh"

// Drawing front end for doghook

namespace sdk {
namespace draw {

struct Color {
    u8 r, g, b, a;
};

enum class RenderTarget {
    surface,
    overlay
};

// initialise the drawing state
void init(RenderTarget t);

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

using FontHandle = u32;

// Register a font
FontHandle register_font(const char *font_name, u32 size);

void text(FontHandle f, Color c, math::Vector p, const char *format, ...);

// TODO: drawing fonts + setting colors

} // namespace draw
} // namespace sdk

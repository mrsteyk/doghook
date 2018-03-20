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

// returns the screen coords of the world space point v
// if the point is not on screen, the return value
// will be equal to math::Vector::invalid()
math::Vector world_to_screen(math::Vector &v);

// TODO: drawing primitives + setting colors

// draw a filled rectangle from p1 to p2
void filled_rect(Color c, math::Vector p1, math::Vector p2);

// draw an outlined rectangle from p1 to p2
void outlined_rect(Color c, math::Vector p1, math::Vector p2);

// TODO: drawing fonts + setting colors

} // namespace draw
} // namespace sdk

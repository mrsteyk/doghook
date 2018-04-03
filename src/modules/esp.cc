#include "precompiled.hh"

#include "esp.hh"

#include "sdk/draw.hh"
#include "sdk/entity.hh"
#include "sdk/player.hh"

#include "utils/profiler.hh"

using namespace sdk;

namespace esp {

draw::FontHandle f = -1;

void show_nodes_recursive(profiler::ProfileNode *node, math::Vector &pos) {
    profiler_profile_function();

    if (node->parent != nullptr) {
        auto parent_time = node->parent->t.milliseconds();

        auto percentage = node->t.milliseconds() / parent_time * 100;

        // Print yourself first
        draw::text(f, {255, 255, 255, 255}, pos, "%s: %f %f%", node->name, node->t.milliseconds(), percentage);
    } else {
        draw::text(f, {255, 255, 255, 255}, pos, "%s: %f", node->name, node->t.milliseconds());
    }

    // now print children
    for (auto &c : node->children) {
        pos.x += 20;
        pos.y += 16.0f;
        show_nodes_recursive(c, pos);
        pos.x -= 20;
    }
}

void show_nodes(profiler::ProfileNode *node, math::Vector pos) {
    show_nodes_recursive(node, pos);
}

void paint() {
    profiler_profile_function();

    if (f == -1) {
        if constexpr (doghook_platform::windows())
            f = draw::register_font("Tahoma", 15);
        else if constexpr (doghook_platform::linux())
            f = draw::register_font("Tahoma", 17);
    }

    //draw::filled_rect({255, 0, 0, 255}, {500, 500, 0}, {1000, 1000, 0});

    draw::text(f, {255, 0, 0, 255}, {0, 0, 0}, "DOGHOOK:tm: :joy: :joy: :jo3: :nice:");

    show_nodes(profiler::find_root_node(), {0, 100, 0});

    if (IFace<Engine>()->in_game() == false) return;

    for (auto e : IFace<EntList>()->get_range()) {
        if (e->is_valid() == false) continue;

        if (auto player = e->to_player()) {
            auto world_space_centre = player->world_space_centre();

            // auto player_min_screen = draw::world_to_screen(render_bounds.first);
            // auto player_max_screen = draw::world_to_screen(render_bounds.second);

            // if (player_min_screen != math::Vector::invalid() && player_max_screen != math::Vector::invalid()) {
            //     draw::filled_rect({255, 255, 255, 255}, player_min_screen, player_max_screen);
            // }

            math::Vector screen_space_centre;
            if (draw::world_to_screen(world_space_centre, screen_space_centre)) {
                //draw::filled_rect({255, 255, 255, 255}, screen_space_centre + math::Vector{-2, -2, -2}, screen_space_centre + math::Vector{2, 2, 2});
                auto info = player->info();
                if (info.user_id != -1 && player->alive())
                    draw::text(f, {255, 255, 255, 255}, screen_space_centre, "%s", info.name);
            }
        }
    }
}

} // namespace esp

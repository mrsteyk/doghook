#include "precompiled.hh"

#include "esp.hh"

#include "sdk/convar.hh"
#include "sdk/draw.hh"
#include "sdk/entity.hh"
#include "sdk/player.hh"
#include "utils/hex.hh"

#include "utils/profiler.hh"

using namespace sdk;

namespace esp {

draw::FontHandle f = -1;

// TODO: do this in a stack based method instead of recursing!
void show_nodes_recursive(profiler::ProfileNode *node, math::Vector &pos) {
    profiler_profile_function();

    if (node->parent != nullptr) {
        auto parent_cycles = node->parent->t.cycles();

        auto percentage = float(node->t.cycles()) / parent_cycles * 100;

        // Print yourself first
        draw::text(f, {255, 255, 255, 255}, pos, "%s: %llu %f %f%", node->name, node->t.cycles(), node->t.milliseconds(), percentage);
    } else {
        draw::text(f, {255, 255, 255, 255}, pos, "%s: %d %f", node->name, node->t.cycles(), node->t.milliseconds());
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

static Convar<const char *> doghook_esp_enemy{"doghook_esp_enemy", "FF0000FF", nullptr};
static Convar<const char *> doghook_esp_friendly{"doghook_esp_friend", "00FF00FF", nullptr};

void paint() {
    profiler_profile_function();

    if (f == -1) {
        if constexpr (doghook_platform::windows())
            f = draw::register_font("Tahoma", 15);
        else if constexpr (doghook_platform::linux())
            f = draw::register_font("Tahoma", 17);
    }

    draw::text(f, {255, 0, 0, 255}, {0, 0, 0}, "DOGHOOK:tm: :joy: :joy: :jo3: :nice:");

    show_nodes(profiler::find_root_node(), {0, 100, 0});

    if (!iface::engine->in_game()) return;

    auto local_player = Player::local();

    auto local_player_team = local_player->team();

    auto friendly_color = draw::Color(hex::dword(doghook_esp_friendly.to_string()));
    auto enemy_color    = draw::Color(hex::dword(doghook_esp_enemy.to_string()));

    for (auto e : iface::ent_list->get_range()) {
        if (!e->is_valid()) continue;

        if (e->dormant()) continue;

        if (auto player = e->to_player()) {
            if (!player->alive()) continue;

            auto player_color = player->team() == local_player_team ? friendly_color : enemy_color;

            auto world_space_centre = player->world_space_centre();

            math::Vector screen_space_centre;
            if (draw::world_to_screen(world_space_centre, screen_space_centre)) {
                auto info = player->info();
                if (info.user_id != -1)
                    draw::text(f, player_color, screen_space_centre, "%s", info.name);
            }

            // These boxes are funky and they need to be transformed better...
            {
                auto collision = player->collision_bounds();

                auto origin = player->origin();

                auto min = collision.first + origin;
                auto max = collision.second + origin;

                math::Vector point_list[] = {
                    {min.x, min.y, min.z},
                    {min.x, max.y, min.z},
                    {max.x, max.y, min.z},
                    {max.x, min.y, min.z},
                    {max.x, max.y, max.z},
                    {min.x, max.y, max.z},
                    {min.x, min.y, max.z},
                    {max.x, min.y, max.z},
                };

                draw::world_to_screen(point_list[0], min);
                max = min;

                auto visible = true;
                for (u32 i = 1; i < 8; i++) {
                    math::Vector new_point;
                    draw::world_to_screen(point_list[i], new_point);

                    min = min.min(new_point);
                    max = max.max(new_point);
                }

                draw::outlined_rect(player_color, min, max);
            }
        }
    }
}

} // namespace esp

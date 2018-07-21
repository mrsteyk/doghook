#include "precompiled.hh"

#include "esp.hh"

#include "sdk/convar.hh"
#include "sdk/draw.hh"
#include "sdk/entity.hh"
#include "sdk/player.hh"
#include "utils/hex.hh"

#include "utils/profiler.hh"

using namespace sdk;

extern Convar<bool> doghook_profiling_enabled;

namespace paint_hook {
extern float engine_vgui_begin_time;
extern float engine_vgui_draw_time;
extern float engine_vgui_finish_time;
} // namespace paint_hook

namespace esp {

draw::FontHandle f = -1;

// TODO: do this in a stack based method instead of recursing!
void show_nodes_recursive(profiler::ProfileNode *node, math::Vector &pos) {

    if (node->parent != nullptr) {
        auto parent_cycles = node->parent->t.cycles();

        auto percentage = float(node->t.cycles()) / parent_cycles * 100;

        // Print yourself first
        draw::text(f, 15, {255, 255, 255, 255}, pos, "%s: %llu %f %f%", node->name, node->t.cycles(), node->t.milliseconds(), percentage);
    } else {
        draw::text(f, 15, {255, 255, 255, 255}, pos, "%s: %d %f", node->name, node->t.cycles(), node->t.milliseconds());
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
    if (doghook_profiling_enabled) show_nodes_recursive(node, pos);
}

static Convar<const char *> doghook_esp_enemy{"doghook_esp_enemy", "FF0000FF", nullptr};
static Convar<const char *> doghook_esp_friendly{"doghook_esp_friend", "00FF00FF", nullptr};

static Convar<bool> doghook_esp_3dbox{"doghook_esp_3dbox", false, nullptr};

void paint() {
    profiler_profile_function();

    if (f == -1) {
        if constexpr (doghook_platform::windows())
            f = draw::register_font("C:\\Windows\\Fonts\\Tahoma.ttf", "Tahoma", 15);
        else if constexpr (doghook_platform::linux())
            f = draw::register_font("/usr/share/fonts/TTF/DejaVuSans.ttf", "DejaVuSans", 17);
    }

    draw::text(f, 15, {255, 0, 0, 255}, {0, 0, 0}, "DOGHOOK:tm: :joy: :joy: :jo3: :nice:");

    draw::text(f, 15, {255, 255, 255, 255}, {0, 15, 0}, " begin_draw: %fms", paint_hook::engine_vgui_begin_time);
    draw::text(f, 15, {255, 255, 255, 255}, {0, 30, 0}, "  draw_time: %fms", paint_hook::engine_vgui_draw_time);
    draw::text(f, 15, {255, 255, 255, 255}, {0, 45, 0}, "finish_draw: %fms", paint_hook::engine_vgui_finish_time);

    // Times are in milliseconds
    auto fps = 1000.0f / (paint_hook::engine_vgui_begin_time +
                          paint_hook::engine_vgui_draw_time +
                          paint_hook::engine_vgui_finish_time);

    draw::text(f, 15, {255, 255, 255, 255}, {0, 60, 0}, "        fps: %f", fps);

    show_nodes(profiler::find_root_node(), {0, 100, 0});

    // Check we are in game before trying to get at entities
    if (!iface::engine->in_game()) return;

    auto local_player      = Player::local();
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
                    draw::text(f, 15, player_color, screen_space_centre, "%s", info.name);
            }

            {
                auto collision = player->collision_bounds();

                auto origin = player->origin();

                auto min = collision.first + origin;
                auto max = collision.second + origin;

                math::Vector point_list[] = {
                    // points on the top
                    {min.x, min.y, min.z},
                    {min.x, max.y, min.z},
                    {max.x, max.y, min.z},
                    {max.x, min.y, min.z},

                    // points on the bottom
                    {min.x, min.y, max.z},
                    {min.x, max.y, max.z},
                    {max.x, max.y, max.z},
                    {max.x, min.y, max.z},
                };

                if (!doghook_esp_3dbox) {
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
                } else {
                    // first transform all the points
                    math::Vector transformed[8];

                    // TODO: benchmark this vs for loop transformation
                    std::transform(point_list, point_list + 8, transformed, [](const math::Vector &x) {
                        math::Vector result;
                        draw::world_to_screen(x, result);
                        return result;
                    });

                    // TODO: check that clang correctly unrolls this and if not then just hardcode
                    // those in

                    // draw the top square and the bottom square
                    {
                        for (int i = 1; i < 4; i++) {
                            draw::line(player_color, transformed[i - 1], transformed[i]);
                        }
                        // draw back to the start now
                        draw::line(player_color, transformed[0], transformed[3]);
                    }

                    {
                        for (int i = 5; i < 8; i++) {
                            draw::line(player_color, transformed[i - 1], transformed[i]);
                        }
                        // draw back to the start now
                        draw::line(player_color, transformed[4], transformed[7]);
                    }

                    // draw vertical lines
                    {
                        for (int i = 0; i < 4; i++) {
                            draw::line(player_color, transformed[0 + i], transformed[4 + i]);
                        }
                    }
                }
            }
        }
    }
}

} // namespace esp

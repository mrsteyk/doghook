#include <precompiled.hh>

#include "anti_aim.hh"

#include <hooks/createmove.hh>

#include <sdk/convar.hh>
#include <sdk/player.hh>
#include <sdk/sdk.hh>

using namespace sdk;

namespace anti_aim {

Convar<bool> doghook_antiaim_enabled{"doghook_antiaim_enabled", false, nullptr};

Convar<float> doghook_antiaim_real_angle{"doghook_antiaim_real_angle", 90, -1000, 1000, nullptr};
Convar<float> doghook_antiaim_fake_angle{"doghook_antiaim_fake_angle", -90, -1000, 1000, nullptr};
Convar<float> doghook_antiaim_pitch{"doghook_antiaim_pitch_angle", 0, -1000, 1000, nullptr};

Convar<bool> doghook_antiaim_force_sendpacket_false{"doghook_antiaim_force_sendpacket_false", false, nullptr};
Convar<bool> doghook_antiaim_add_to_angles{"doghook_antiaim_add_to_angles", true, nullptr};
Convar<bool> doghook_antiaim_set_pitch{"doghook_antiaim_set_pitch", true, nullptr};

auto send_packet_flip = false;

inline static auto clamp_angle(const math::Vector &angles) {
    math::Vector out;

    out.x = angles.x;
    out.y = angles.y;
    out.z = 0;

    while (out.x > 89.0f) out.x -= 180.0f;
    while (out.x < -89.0f) out.x += 180.0f;

    while (out.y > 180.0f) out.y -= 360.0f;
    while (out.y < -180.0f) out.y += 360.0f;

    out.y = std::clamp(out.y, -180.0f, 180.0f);
    out.x = std::clamp(out.x, -90.0f, 90.0f);

    return out;
}

inline static auto fix_movement_for_new_angles(const math::Vector &movement, const math::Vector &old_angles, const math::Vector &new_angles) {
    math::Matrix3x4 rotate_matrix;

    auto delta_angles = new_angles - old_angles;
    delta_angles.x    = 0; // We dont care about pitch for this

    rotate_matrix.from_angle(delta_angles);
    return rotate_matrix.rotate_vector(movement);
}

void create_move(sdk::UserCmd *cmd) {
    auto send_packet = create_move::send_packet();

    //auto local_player = Player::local();
    //*(u32 *)((u8 *)local_player + 2120) = 0;

    if (!(cmd->buttons & IN_ATTACK) && doghook_antiaim_enabled) {
        auto old_angles = cmd->viewangles;

        if (send_packet_flip) {
            if (doghook_antiaim_add_to_angles)
                cmd->viewangles.y += doghook_antiaim_real_angle;
            else
                cmd->viewangles.y = doghook_antiaim_real_angle;
        } else {
            if (doghook_antiaim_add_to_angles)
                cmd->viewangles.y += doghook_antiaim_fake_angle;
            else
                cmd->viewangles.y = doghook_antiaim_fake_angle;
        }

        if (doghook_antiaim_set_pitch)
            cmd->viewangles.x = doghook_antiaim_pitch;
        //cmd->viewangles.x = -90.0f;

        //clamp_angle(cmd->viewangles);
        auto new_movement = fix_movement_for_new_angles({cmd->forwardmove, cmd->sidemove, 0}, old_angles, cmd->viewangles);
        cmd->forwardmove  = new_movement.x;
        cmd->sidemove     = new_movement.y;

        *send_packet     = send_packet_flip;
        send_packet_flip = !send_packet_flip;

        if (doghook_antiaim_force_sendpacket_false) *send_packet = false;
    }
}
} // namespace anti_aim

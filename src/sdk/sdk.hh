#pragma once

#include "types.hh"
#include "vfunc.hh"

#include "datatable.hh"
#include "entity.hh"
#include "interface.hh"
#include "signature.hh"

#include "inputcodes.hh"
#include "trace.hh"

namespace sdk {
class Player;

class UserCmd {
public:
    virtual ~UserCmd(){};

    int          command_number;
    int          tick_count;
    math::Vector viewangles;
    float        forwardmove;
    float        sidemove;
    float        upmove;
    int          buttons;
    u8           impulse;
    int          weapon_select;
    int          weapon_subtype;
    int          random_seed;
    short        mouse_dx;
    short        mouse_dy;
    bool         has_been_predicted;
};

struct ClientClass {
    void *       create_fn;
    void *       create_event_fn; // Only called for event objects.
    const char * network_name;
    RecvTable *  recv_table;
    ClientClass *next;
    int          class_id; // Managed by the engine.
};

class Client {
public:
    Client() = delete;

    ClientClass *get_all_classes() {
        return_virtual_func(get_all_classes, 8, 8, 8, 0);
    }
};

class ClientMode {
public:
    ClientMode() = delete;
};

class NetChannel {
public:
    enum class Flow {
        outgoing,
        incoming
    };

    NetChannel() = delete;

    float latency(Flow f) {
        return_virtual_func(latency, 9, 9, 9, 0, f);
    }

    i32 sequence_number(Flow f) {
        return_virtual_func(sequence_number, 16, 16, 16, 0, f);
    }

    i32 &queued_packets() {
        // TODO: this should be auto when implemented for all platforms
        static u32 queued_packets_offset = []() {
            if constexpr (doghook_platform::windows()) {
                return *signature::find_pattern<u32 *>("engine", "83 BE ? ? ? ? ? 0F 9F C0 84 C0", 2);
            } else if constexpr (doghook_platform::linux()) {
                return 0;
            } else if constexpr (doghook_platform::osx()) {
                return 0;
            }
        }();

        assert(queued_packets_offset);

        auto &queued_packets = *reinterpret_cast<i32 *>(reinterpret_cast<u8 *>(this) + queued_packets_offset);

        return queued_packets;
    }

    // Below this point are functions that use offsets
    // These offsets should be consistent accross multiple platforms
    // As the netchannel structure is the same

    template <typename T, u32 offset>
    auto &get() { return *reinterpret_cast<T *>(reinterpret_cast<uptr>(this) + offset); }

    auto &out_sequence() { return get<u32, 8>(); }
    auto &in_sequence() { return get<u32, 12>(); }
    auto &in_reliable_state() { return get<u32, 24>(); }
    auto &out_reliable_state() { return get<u32, 20>(); }
};

class Globals {
public:
    float realtime;
    int   framecount;

    float absolute_frametime;
    float curtime;

    float frametime;

    int max_clients;

    int   tickcount;
    float interval_per_tick;

    float interpolation_amount;
    int   sim_ticks_this_frame;

    int network_protocol;

    class CSaveRestoreData *save_data;

    bool is_client;

    i32   time_to_ticks(float time);
    float ticks_to_time(i32 ticks);
};

struct PlayerInfo;

class Engine {
public:
    Engine() = delete;

    float last_timestamp() {
        return_virtual_func(last_timestamp, 15, 15, 15, 0);
    }

    float time() {
        return_virtual_func(time, 14, 14, 14, 0);
    }

    bool in_game() {
        return_virtual_func(in_game, 26, 26, 26, 0);
    }

    u32 local_player_index() {
        return_virtual_func(local_player_index, 12, 12, 12, 0);
    }

    NetChannel *net_channel_info() {
        return_virtual_func(net_channel_info, 72, 72, 72, 0);
    }

    bool is_box_visible(const math::Vector &min, const math::Vector &max) {
        return_virtual_func(is_box_visible, 31, 31, 31, 0, min, max);
    }

    u32 max_clients() {
        return_virtual_func(max_clients, 21, 21, 21, 0);
    }

    void set_view_angles(const math::Vector &v) {
        return_virtual_func(set_view_angles, 20, 20, 20, 0, v);
    }

    bool player_info(u32 index, PlayerInfo *info) {
        return_virtual_func(player_info, 8, 8, 8, 0, index, info);
    }
};

class EntList {
public:
    EntList() = delete;

    Entity *entity(u32 index) {
        return_virtual_func(entity, 3, 3, 3, 0, index);
    }

    Entity *from_handle(EntityHandle h) {

        if constexpr (doghook_platform::windows()) {
            return_virtual_func(from_handle, 4, 4, 4, 0, h);
        } else if constexpr (doghook_platform::linux())
            return entity(h.serial_index & 0xFFF);
    }

    u32 max_entity_index() {
        return_virtual_func(max_entity_index, 6, 6, 6, 0);
    }

    class EntityRange {
        EntList *parent;

        u32 max_entity;

    public:
        class Iterator {
            u32      index;
            EntList *parent;

        public:
            // TODO: should we use 1 here or should we use 0 and force people
            // that loop to deal with that themselves...
            Iterator(EntList *parent) : index(1), parent(parent) {}
            explicit Iterator(u32 index, EntList *parent)
                : index(index), parent(parent) {}

            auto &operator++() {
                ++index;
                return *this;
            }

            auto operator*() {
                return parent->entity(index);
            }

            auto operator==(const Iterator &b) {
                return index == b.index;
            }

            auto operator!=(const Iterator &b) {
                return !(*this == b);
            }
        };

        EntityRange(EntList *parent) : parent(parent), max_entity(parent->max_entity_index()) {}

        explicit EntityRange(EntList *parent, u32 max_entity) : parent(parent), max_entity(max_entity) {}

        auto begin() { return Iterator(parent); }

        auto end() { return Iterator(max_entity, parent); }
    };

    auto get_range() { return EntityRange(this); }
    auto get_range(u32 max_entity) { return EntityRange(this, max_entity + 1); }

}; // namespace sdk

class Input {
public:
    Input() = delete;

    UserCmd *get_user_cmd(u32 sequence_number) {
        // Look at above queued_packets_offset for more info
        static u32 array_offset = []() {
            if constexpr (doghook_platform::windows()) {
                return *signature::find_pattern<u32 *>("client", "8B 87 ? ? ? ? 8B CA", 2);
            } else if constexpr (doghook_platform::linux()) {
                return 0;
            } else if constexpr (doghook_platform::osx()) {
                return 0;
            }
        }();
        // this should not be 0
        assert(array_offset != 0);

        auto cmd_array = *reinterpret_cast<UserCmd **>(reinterpret_cast<u8 *>(this) + array_offset);

        return &cmd_array[sequence_number % 90];
    }

    class VerifiedCmd *get_verified_user_cmd(u32 sequence_number) {
        // 03 B7 ? ? ? ? 8D 04 88
        return nullptr;
    }
};

// defined in convar.cc
class ConCommandBase;

class Cvar {
public:
    Cvar() = delete;

    u32 allocate_dll_identifier() {
        return_virtual_func(allocate_dll_identifier, 5, 5, 5, 0);
    }

    void register_command(ConCommandBase *command) {
        return_virtual_func(register_command, 6, 6, 6, 0, command);
    }

    void unregister_command(ConCommandBase *command) {
        return_virtual_func(unregister_command, 7, 7, 7, 0, command);
    }

    ConCommandBase *root_node() {
        return_virtual_func(root_node, 16, 16, 16, 0);
    }

    ConCommandBase *find_var(const char *name) {
        return_virtual_func(find_var, 12, 12, 12, 0, name);
    }
};

class Trace {
public:
    Trace() = delete;

    void trace_ray(const trace::Ray &ray, u32 mask, trace::Filter *filter, trace::TraceResult *results) {
        return_virtual_func(trace_ray, 4, 4, 4, 0, ray, mask, filter, results);
    }
};

// These are defined to give us distinct types for these
// very different objects, to help us differentiate between them

class ModelHandle; // model_t

// mstudiobbox
class StudioHitbox {
public:
    StudioHitbox() = delete;

    int bone;
    int group;

    math::Vector min;
    math::Vector max;

    int name_index;

private:
    int unused[8];
};

// mstudiobboxset
class StudioHitboxSet {
public:
    StudioHitboxSet() = delete;

    int  name_index;
    auto name() const {
        assert(0);
        return "";
    }

    u32 hitboxes_count;
    u32 hitbox_index;

    const auto operator[](u32 index) const {
        assert(index < hitboxes_count);

        return reinterpret_cast<const StudioHitbox *>(
                   reinterpret_cast<const u8 *>(this) + hitbox_index) +
               index;
    }
};

// studiohdr_t
class StudioModel {
public:
    StudioModel() = delete;

    int  id;
    int  version;
    int  checksum;
    char name[64];
    int  length;

    math::Vector eye_position; // ideal eye position

    math::Vector illumination_position; // illumination center

    math::Vector hull_min; // ideal movement hull size
    math::Vector hull_max;

    math::Vector view_bbmin; // clipping bounding box
    math::Vector view_bbmax;

    int flags;

    int bones_count;
    int bone_index;

    int bone_controllers_count;
    int bone_controller_index;

    u32 hitbox_sets_count;
    u32 hitbox_set_index;

    const auto hitbox_set(u32 index) const {
        assert(index < hitbox_sets_count);

        return reinterpret_cast<const StudioHitboxSet *>(
                   reinterpret_cast<const u8 *>(this) + hitbox_set_index) +
               index;
    }

    //... TODO:
};

class StudioFullModel {
public:
    StudioFullModel() = delete;
};

class ModelInfo {
public:
    ModelInfo() = delete;

    const char *model_name(const ModelHandle *m) {
        return_virtual_func(model_name, 3, 4, 4, 0, m);
    }

    const StudioModel *studio_model(const ModelHandle *m) {
        return_virtual_func(studio_model, 28, 29, 29, 0, m);
    }

    const StudioFullModel *extra_data(const ModelHandle *m) {
        return_virtual_func(extra_data, 10, 11, 11, 0, m);
    }
};

// This should only be used for debugging.
// For real output use the overlay
class DebugOverlay {
public:
    using OverlayText_t = void;

    virtual void add_entity_text_overlay(int ent_index, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...)                                                                         = 0;
    virtual void add_box_overlay(const math::Vector &origin, const math::Vector &mins, const math::Vector &max, const math ::Vector &orientation, int r, int g, int b, int a, float duration)                         = 0;
    virtual void add_triangle_overlay(const math::Vector &p1, const math::Vector &p2, const math::Vector &p3, int r, int g, int b, int a, bool no_depth_test, float duration)                                         = 0;
    virtual void add_line_overlay(const math::Vector &origin, const math::Vector &dest, int r, int g, int b, bool no_depth_test, float duration)                                                                      = 0;
    virtual void add_text_overlay(const math::Vector &origin, float duration, const char *format, ...)                                                                                                                = 0;
    virtual void add_text_overlay(const math::Vector &origin, int line_offset, float duration, const char *format, ...)                                                                                               = 0;
    virtual void add_screen_text_overlay(float x_pos, float y_pos, float duration, int r, int g, int b, int a, const char *text)                                                                                      = 0;
    virtual void add_swept_box_overlay(const math::Vector &start, const math::Vector &end, const math::Vector &mins, const math::Vector &max, const math::Vector &angles, int r, int g, int b, int a, float duration) = 0;
    virtual void add_grid_overlay(const math::Vector &origin)                                                                                                                                                         = 0;
    virtual int  screen_position(const math::Vector &point, math::Vector &screen)                                                                                                                                     = 0;
    virtual int  screen_position(float x_pos, float y_pos, math::Vector &screen)                                                                                                                                      = 0;

    virtual OverlayText_t *get_first(void)                  = 0;
    virtual OverlayText_t *get_next(OverlayText_t *current) = 0;
    virtual void           clear_dead_overlays()            = 0;
    virtual void           clear_all_overlays()             = 0;

    virtual void add_text_overlay_rgb(const math::Vector &origin, int line_offset, float duration, float r, float g, float b, float alpha, const char *format, ...) = 0;
    virtual void add_text_overlay_rgb(const math::Vector &origin, int line_offset, float duration, int r, int g, int b, int a, const char *format, ...)             = 0;

    virtual void add_line_overlay_alpha(const math::Vector &origin, const math::Vector &dest, int r, int g, int b, int a, bool noDepthTest, float duration) = 0;
    //virtual void add_box_overlay2(const math::Vector &origin, const math::Vector &mins, const math::Vector &max, const math::Vector &orientation, const Color &faceColor, const Color &edgeColor, float duration) = 0;
};

// This is from server.dll
class PlayerInfoManager {
public:
    Globals *globals() {
        return_virtual_func(globals, 1, 1, 1, 0);
    }
};

class MoveHelper;

class Prediction {
public:
    void setup_move(Player *player, UserCmd *ucmd, MoveHelper *helper, void *move) {
        return_virtual_func(setup_move, 18, 19, 19, 0, player, ucmd, helper, move);
    }
    void finish_move(Player *player, UserCmd *ucmd, void *move) {
        return_virtual_func(finish_move, 19, 20, 20, 0, player, ucmd, move);
    }
};

class GameMovement {
public:
    void process_movement(Player *player, void *move) {
        return_virtual_func(process_movement, 1, 2, 2, 0, player, move);
    }
};

class AnimationLayer {
public:
    int   sequence;
    float prev_cycle;
    float weight;
    int   order;

    float playback_rate;
    float cycle;

    float layer_anim_time;
    float layer_fade_time;

    float blend_in;
    float blend_out;

    bool client_blend;
};

class EngineVgui {
public:
};

class InputSystem {
public:
    bool is_button_down(ButtonCode code) {
        return_virtual_func(is_button_down, 11, 11, 11, 0, code);
    }
    const char *button_code_to_string(ButtonCode code) {
        return_virtual_func(button_code_to_string, 27, 27, 27, 0, code);
    }
    ButtonCode string_to_button_code(const char *string) {
        return_virtual_func(string_to_button_code, 29, 29, 29, 0, string);
    }
};

// Wrappers around the IFace<type> system: iface::<type>
namespace iface {
extern IFace<Client>            client;
extern IFace<ClientMode>        client_mode;
extern IFace<Globals>           globals;
extern IFace<Engine>            engine;
extern IFace<EntList>           ent_list;
extern IFace<Input>             input;
extern IFace<Cvar>              cvar;
extern IFace<Trace>             trace;
extern IFace<ModelInfo>         model_info;
extern IFace<DebugOverlay>      overlay;
extern IFace<PlayerInfoManager> info_manager;
extern IFace<Prediction>        prediction;
extern IFace<MoveHelper>        move_helper;
extern IFace<GameMovement>      game_movement;
extern IFace<EngineVgui>        engine_vgui;
extern IFace<InputSystem>       input_system;

// TODO: update to use type aliases instead of extern vars
// using client        = IFace<Client>;
// using client_mode   = IFace<ClientMode>;
// using cvar          = IFace<Cvar>;
// using engine        = IFace<Engine>;
// using engine_vgui   = IFace<EngineVgui>;
// using ent_list      = IFace<EntList>;
// using game_movement = IFace<GameMovement>;
// using globals       = IFace<Globals>;
// using info_manager  = IFace<PlayerInfoManager>;
// using input         = IFace<Input>;
// using input_system  = IFace<InputSystem>;
// using model_info    = IFace<ModelInfo>;
// using move_helper   = IFace<MoveHelper>;
// using overlay       = IFace<DebugOverlay>;
// using prediction    = IFace<Prediction>;
// using trace         = IFace<Trace>;
} // namespace iface

} // namespace sdk

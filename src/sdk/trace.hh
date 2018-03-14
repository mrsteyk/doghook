#pragma once

#include "platform.hh"

namespace trace {
class VectorAligned : public math::Vector {
    float pad;

public:
    VectorAligned() {}

    explicit VectorAligned(const math::Vector &other) {
        x = other.x;
        y = other.y;
        z = other.z;
    }

    explicit VectorAligned(const math::Vector &&other) {
        x = other.x;
        y = other.y;
        z = other.z;
    }

    auto operator=(const Vector &other) {
        x = other.x;
        y = other.y;
        z = other.z;

        return *this;
    }

    auto operator=(const Vector &&other) {
        x = other.x;
        y = other.y;
        z = other.z;

        return *this;
    }
};

struct Plane {
    math::Vector normal;
    float        dist;
    u8           type;     // for fast side tests
    u8           signbits; // signx + (signy<<1) + (signz<<1)
    u8           pad[2];
};

struct Model {
    math::Vector min, max;
    math::Vector origin; // for sounds or lights
    int          head_node;

    struct VCollide {
        unsigned short solidCount : 15;
        unsigned short isPacked : 1;
        unsigned short descSize;
        // VPhysicsSolids
        void **solids;
        char * pKeyValues;
    };

    VCollide vcollision_data;
};

struct Surface {
    const char *   name;
    short          surface_props;
    unsigned short flags;
};

struct TraceResult {
    math::Vector start_pos; // start position
    math::Vector end_pos;   // final position
    Plane        plane;     // surface normal at impact

    float fraction; // time completed, 1.0 = didn't hit anything

    int            contents;   // contents on other side of surface hit
    unsigned short disp_flags; // displacement flags for marking surfaces with data

    bool all_solid;   // if true, plane is not valid
    bool start_solid; // if true, the initial point was in a solid area

    float   fraction_left_solid; // time we left a solid, only valid if we started in solid
    Surface surface;             // surface hit (impact surface)

    int   hitgroup;    // 0 == generic, non-zero is specific body part
    short physicsbone; // physics bone hit by trace in studio

    sdk::Entity *entity;

    // NOTE: this member is overloaded.
    // If hEnt points at the world entity, then this is the static prop index.
    // Otherwise, this is the hitbox index.
    int hitbox; // box hit by trace in studio
};

class Ray {
public:
    VectorAligned start;
    VectorAligned delta;
    VectorAligned start_offset;
    VectorAligned extents;
    bool          is_ray;
    bool          is_swept;

    // init for point ray
    auto init(const math::Vector &start, const math::Vector &end) {
        delta    = end - start;
        is_swept = (delta.length_sqr() != 0);
        is_ray   = true;

        extents      = math::Vector::zero();
        start_offset = math::Vector::zero();

        this->start = start;
    }

    // init for box ray
    auto init(const math::Vector &start, const math::Vector &end, const math::Vector &min, const math::Vector &max) {
        delta = end - start;

        is_swept = (delta.length_sqr() != 0);

        extents = max - min;
        extents *= 0.5;
        is_ray = false;

        start_offset = min + max;
        start_offset *= 0.5f;

        this->start = start_offset + start;
        start_offset *= -1.0f;
    }
};

class Filter {
    sdk::Entity *ignore_self;
    sdk::Entity *ignore_entity;

public:
    virtual auto should_hit_entity(sdk::Entity *handle_entity, int contents_mask) -> bool;
    virtual auto GetTraceType() const -> u32 {
        return 0; // hit everything
    }

    Filter() : ignore_self(nullptr), ignore_entity(nullptr) {}

    Filter(sdk::Entity *ignore_self) : ignore_self(ignore_self) {}
    Filter(sdk::Entity *ignore_self, sdk::Entity *ignore_entity) : ignore_self(ignore_self), ignore_entity(ignore_entity) {}
};

} // namespace trace

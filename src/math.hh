#pragma once

#include "platform.hh"

#include <cassert>
#include <cmath>
#include <float.h>

#undef max
#undef min

namespace math {
constexpr auto PI = 3.14159265358979323846f;

inline auto to_radians(float x) {
    return x * (PI / 180.0f);
}

inline auto to_degrees(float x) {
    return x * (180.0f / PI);
}

template <typename T>
auto lerp(float percent, T min, T max) {
    return min + ((max - min) * percent);
}

class Vector {
public:
    float x, y, z;

    // sentinal values
    inline static auto origin() { return Vector(0); }
    inline static auto zero() { return Vector(0); }
    inline static auto invalid() { return Vector(FLT_MAX); }

    inline Vector() {}
    inline Vector(float x, float y, float z) : x(x), y(y), z(z) {}
    inline Vector(float v) : x(v), y(v), z(v) {}
    inline Vector(const Vector &v) : x(v.x), y(v.y), z(v.z) {}

    // equality
    inline auto operator==(const Vector &v) const {
        return (x == v.x) && (y == v.y) && (z == v.z);
    }
    inline auto operator!=(const Vector &v) const {
        return (x != v.x) || (y != v.y) || (z != v.z);
    }

    // arithmetic operations
    inline auto operator+=(const Vector &v) {
        x += v.x;
        y += v.y;
        z += v.z;
        return *this;
    }
    inline auto operator-=(const Vector &v) {
        x -= v.x;
        y -= v.y;
        z -= v.z;
        return *this;
    }
    inline auto operator*=(const Vector &v) {
        x *= v.x;
        y *= v.y;
        z *= v.z;
        return *this;
    }
    inline auto operator*=(float s) {
        x *= s;
        y *= s;
        z *= s;
        return *this;
    }
    inline auto operator/=(const Vector &v) {
        assert(v.x != 0.0f && v.y != 0.0f && v.z != 0.0f);
        x /= v.x;
        y /= v.y;
        z /= v.z;
        return *this;
    }
    inline auto operator/=(float s) {
        assert(s != 0.0f);
        float oofl = 1.0f / s;
        x *= oofl;
        y *= oofl;
        z *= oofl;
        return *this;
    }
    inline auto operator-(void) const {
        return Vector(-x, -y, -z);
    }
    inline auto operator+(const Vector &v) const {
        return Vector(*this) += v;
    }
    inline auto operator-(const Vector &v) const {
        return Vector(*this) -= v;
    }
    auto operator*(const Vector &v) const {
        return Vector(*this) *= v;
    }
    inline auto operator/(const Vector &v) const {
        return Vector(*this) /= v;
    }
    inline auto operator*(float s) const {
        return Vector(*this) *= s;
    }
    inline auto operator/(float s) const {
        return Vector(*this) /= s;
    }

    inline auto length_sqr() const {
        return (x * x) + (y * y) + (z * z);
    }
    inline auto length() const {
        return std::sqrt(length_sqr());
    }
    inline auto distance(const Vector &b) const {
        auto d = *this - b;
        return d.length();
    }
    inline auto dot(Vector &b) const {
        return (x * b.x) + (y * b.y) + (z * b.z);
    }
    inline auto cross(Vector &v) const {
        auto res = Vector::invalid();
        res.x    = y * v.z - z * v.y;
        res.y    = z * v.x - x * v.z;
        res.z    = x * v.y - y * v.x;
        return res;
    }

    inline auto to_angle() const {
        float tmp, yaw, pitch;

        if (y == 0 && x == 0) {
            yaw = 0;
            if (z > 0)
                pitch = 270;
            else
                pitch = 90;
        } else {
            yaw = (atan2(y, x) * 180 / PI);
            if (yaw < 0)
                yaw += 360;

            tmp   = sqrt(x * x + y * y);
            pitch = (atan2(-z, tmp) * 180 / PI);
            if (pitch < 0)
                pitch += 360;
        }

        return Vector(pitch, yaw, 0);
    }

    inline auto to_vector() const {
        float sp, sy, cp, cy;

        sy = sinf(to_radians(y));
        cy = cosf(to_radians(y));

        sp = sinf(to_radians(x));
        cp = cosf(to_radians(x));

        return Vector(cp * cy, cp * sy, -sp);
    }

    inline auto lerp(const Vector &other, float percent) const {
        Vector ret;

        ret.x = x + (other.x - x) * percent;
        ret.y = y + (other.y - y) * percent;
        ret.z = z + (other.z - z) * percent;

        return ret;
    }
};

class Matrix3x4 {
public:
    Matrix3x4() {}
    Matrix3x4(
        float m00, float m01, float m02, float m03,
        float m10, float m11, float m12, float m13,
        float m20, float m21, float m22, float m23) {
        data[0][0] = m00;
        data[0][1] = m01;
        data[0][2] = m02;
        data[0][3] = m03;
        data[1][0] = m10;
        data[1][1] = m11;
        data[1][2] = m12;
        data[1][3] = m13;
        data[2][0] = m20;
        data[2][1] = m21;
        data[2][2] = m22;
        data[2][3] = m23;
    }

    // Creates a matrix where the X axis = forward
    // the Y axis = left, and the Z axis = up
    void init(const Vector &x_axis, const Vector &y_axis, const Vector &z_axis, const Vector &origin) {
        data[0][0] = x_axis.x;
        data[0][1] = y_axis.x;
        data[0][2] = z_axis.x;
        data[0][3] = origin.x;
        data[1][0] = x_axis.y;
        data[1][1] = y_axis.y;
        data[1][2] = z_axis.y;
        data[1][3] = origin.y;
        data[2][0] = x_axis.z;
        data[2][1] = y_axis.z;
        data[2][2] = z_axis.z;
        data[2][3] = origin.z;
    }

    // Creates a matrix where the X axis = forward
    // the Y axis = left, and the Z axis = up
    Matrix3x4(const Vector &x_axis, const Vector &y_axis, const Vector &z_axis, const Vector &origin) {
        init(x_axis, y_axis, z_axis, origin);
    }

    inline void invalidate(void) {
        assert(0);
        // for (int i = 0; i < 3; i++) {
        //     for (int j = 0; j < 4; j++) {
        //         data[i][j] = VEC_T_NAN;
        //     }
        // }
    }

    float *operator[](int i) {
        assert((i >= 0) && (i < 3));
        return data[i];
    }
    const float *operator[](int i) const {
        assert((i >= 0) && (i < 3));
        return data[i];
    }
    float *      base() { return &data[0][0]; }
    const float *base() const { return &data[0][0]; }

    const auto get_column(u32 column, Vector &out) const {
        out.x = data[0][column];
        out.y = data[1][column];
        out.z = data[2][column];
    }

    const auto get_row(u32 row, Vector &out) const {
        out.x = data[row][0];
        out.y = data[row][1];
        out.z = data[row][2];
    }

    auto rotate_vector(const Vector &in) const {
        Vector out;
        Vector row;

        out.x = in.dot((get_row(0, row), row));
        out.y = in.dot((get_row(1, row), row));
        out.z = in.dot((get_row(2, row), row));

        return out;
    }

    auto from_angle(const Vector &angles) {
        auto radian_pitch = to_radians(angles.x);
        auto radian_yaw   = to_radians(angles.y);
        auto radian_roll  = to_radians(angles.z);

        auto sp = sin(radian_pitch);
        auto cp = cos(radian_pitch);

        auto sy = sin(radian_yaw);
        auto cy = cos(radian_yaw);

        auto sr = sin(radian_roll);
        auto cr = cos(radian_roll);

        // matrix = (YAW * PITCH) * ROLL
        data[0][0] = cp * cy;
        data[1][0] = cp * sy;
        data[2][0] = -sp;

        float crcy = cr * cy;
        float crsy = cr * sy;
        float srcy = sr * cy;
        float srsy = sr * sy;
        data[0][1] = sp * srcy - crsy;
        data[1][1] = sp * srsy + crcy;
        data[2][1] = sr * cp;

        data[0][2] = (sp * crcy + srsy);
        data[1][2] = (sp * crsy - srcy);
        data[2][2] = cr * cp;

        data[0][3] = 0.0f;
        data[1][3] = 0.0f;
        data[2][3] = 0.0f;
    }

    float data[3][4];
};

// TODO: make these member functions
inline void matrix_angles(const Matrix3x4 &matrix, float *angles) {
    float forward[3];
    float left[3];
    float up[3];

    //
    // Extract the basis vectors from the matrix. Since we only need the Z
    // component of the up vector, we don't get X and Y.
    //
    forward[0] = matrix[0][0];
    forward[1] = matrix[1][0];
    forward[2] = matrix[2][0];
    left[0]    = matrix[0][1];
    left[1]    = matrix[1][1];
    left[2]    = matrix[2][1];
    up[2]      = matrix[2][2];

    float xyDist = std::sqrt(forward[0] * forward[0] + forward[1] * forward[1]);

    // enough here to get angles?
    if (xyDist > 0.001f) {
        // (yaw)    y = ATAN( forward.y, forward.x );       -- in our space, forward is the X axis
        angles[1] = to_degrees(atan2f(forward[1], forward[0]));

        // (pitch)  x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
        angles[0] = to_degrees(atan2f(-forward[2], xyDist));

        // (roll)   z = ATAN( left.z, up.z );
        angles[2] = to_degrees(atan2f(left[2], up[2]));
    } else {
        // (yaw)    y = ATAN( -left.x, left.y );            -- forward is mostly z, so use right for yaw
        angles[1] = to_degrees(atan2f(-left[0], left[1]));

        // (pitch)  x = ATAN( -forward.z, sqrt(forward.x*forward.x+forward.y*forward.y) );
        angles[0] = to_degrees(atan2f(-forward[2], xyDist));

        // Assume no roll in this case as one degree of freedom has been lost (i.e. yaw == roll)
        angles[2] = 0;
    }
}

inline void matrix_angles(const Matrix3x4 &matrix, Vector &angles, Vector &position) {
    matrix.get_column(3, position);
    matrix_angles(matrix, &angles.x);
}

} // namespace math

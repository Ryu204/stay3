module;

#include <cassert>

// clang-format off
#include <Jolt/Jolt.h>
// clang-format on
#include <Jolt/Core/Color.h>
#include <Jolt/Math/Float2.h>
#include <Jolt/Math/Float3.h>
#include <Jolt/Math/Quat.h>
#include <Jolt/Math/Real.h>
#include <Jolt/Math/Vec3.h>

export module stay3.physics.convert;

import stay3.core;

export namespace st {

namespace priv {
vec4f convert(const JPH::Vec4 &val) {
    return {val.GetX(), val.GetY(), val.GetZ(), val.GetW()};
}
} // namespace priv

JPH::Vec3 convert(const vec3f &val) {
    return {val.x, val.y, -val.z};
}
vec3f convert(const JPH::Vec3 &val) {
    return {val.GetX(), val.GetY(), -val.GetZ()};
}
vec3f convert(const JPH::Float3 &val) {
    return {val[0], val[1], -val[2]};
}
vec2f convert(const JPH::Float2 &val) {
    return {val.x, val.y};
}
quaternionf convert(const JPH::Quat &val) {
    return {-val.GetW(), val.GetX(), val.GetY(), -val.GetZ()};
}
JPH::Quat convert(const quaternionf &val) {
    return {val.x, val.y, -val.z, -val.w};
}
vec4f convert(const JPH::Color &val) {
    return {val.r, val.g, val.b, val.a};
}

mat4f convert(const JPH::RMat44 &val) {
    mat4f result{
        priv::convert(val.GetColumn4(0)),
        priv::convert(val.GetColumn4(1)),
        priv::convert(-val.GetColumn4(2)),
        priv::convert(val.GetColumn4(3)),
    };
    result[0][2] *= -1.F;
    result[1][2] *= -1.F;
    result[2][2] *= -1.F;
    result[3][2] *= -1.F;
    return result;
}
} // namespace st
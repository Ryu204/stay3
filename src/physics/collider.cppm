module;

#include <cassert>
#include <variant>
// clang-format off
#include <Jolt/Jolt.h>
// clang-format on
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

export module stay3.physics:collider;

import stay3.core;

export namespace st {
struct box {
    static constexpr float default_size = 1.F;
    // NOLINTNEXTLINE(*-easily-swappable-parameters)
    box(float sx = default_size, float sy = default_size, float sz = default_size)
        : x{sx}, y{sy}, z{sz} {}
    box(const vec3f &size)
        : box(size.x, size.y, size.z) {}

    float x;
    float y;
    float z;
};

class collider {
    using collider_info = std::variant<box>;

public:
    collider(const collider_info &info)
        : shape_settings{std::visit(init_visitor, info)} {
        std::visit(visit_helper{[](const auto &data) -> void { data.SetEmbedded(); }}, shape_settings);
    }
    [[nodiscard]] JPH::ShapeSettings::ShapeResult build_geometry() const {
        auto result = std::visit(
            visit_helper{
                [](const auto &data) { return data.Create(); },
            },
            shape_settings);
        assert(!result.IsEmpty() && "Invalid physics shape");
        return result;
    }

private:
    using jolt_settings = std::variant<JPH::BoxShapeSettings>;
    static constexpr visit_helper init_visitor{
        [](const box &box) -> jolt_settings { return {JPH::Vec3{box.x, box.y, box.z}}; },
    };
    jolt_settings shape_settings;
};

} // namespace st
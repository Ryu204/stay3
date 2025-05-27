module;

#include <cassert>
#include <variant>
// clang-format off
#include <Jolt/Jolt.h>
// clang-format on
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

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

struct sphere {
    static constexpr auto default_radius = 1.F;
    float radius{default_radius};
};

class collider {
public:
    using info = std::variant<box, sphere>;
    collider(const info &info)
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
    using jolt_settings = std::variant<JPH::BoxShapeSettings, JPH::SphereShapeSettings>;
    static constexpr visit_helper init_visitor{
        [](const box &box) -> jolt_settings {
            return jolt_settings{
                std::in_place_type<JPH::BoxShapeSettings>,
                JPH::Vec3{box.x / 2.F, box.y / 2.F, box.z / 2.F}};
        },
        [](const sphere &sphere) -> jolt_settings {
            return jolt_settings{
                std::in_place_type<JPH::SphereShapeSettings>,
                sphere.radius};
        },
    };
    jolt_settings shape_settings;
};

} // namespace st
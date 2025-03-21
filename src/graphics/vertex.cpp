module;

#include <cstdint>
#include <optional>
#include <vector>

module stay3.graphics;

import stay3.core;
import stay3.ecs;

namespace st {

vec4f color_or_random(const std::optional<vec4f> &color) {
    return color.has_value() ? *(color) : random_color();
}

mesh_data mesh_plane(const vec2f &size, const std::optional<vec4f> &color) {
    mesh_data result;
    // 01
    // 32
    result.vertices.reserve(4);
    result.maybe_indices = std::vector<std::uint32_t>{0, 2, 1, 0, 3, 2};

    result.vertices.emplace_back(color_or_random(color), vec3f{-size.x / 2, size.y / 2, 0.F}, vec_back, vec2f{0.F, 0.F});
    result.vertices.emplace_back(color_or_random(color), vec3f{size.x / 2, size.y / 2, 0.F}, vec_back, vec2f{1.F, 0.F});
    result.vertices.emplace_back(color_or_random(color), vec3f{size.x / 2, -size.y / 2, 0.F}, vec_back, vec2f{1.F, 1.F});
    result.vertices.emplace_back(color_or_random(color), vec3f{-size.x / 2, -size.y / 2, 0.F}, vec_back, vec2f{0.F, 1.F});

    return result;
}

mesh_data mesh_cube(const vec3f &size, const std::optional<vec4f> &color) {
    // NOLINTBEGIN(*-magic-numbers)
    //   04(16)-------------------15(20)
    //  /                     z->/|
    // 3(12)(17)--x----(21)(13)2 |
    // |                        | |
    // |                      y | |
    // | 78(19)                 | 69(23)
    // |                        |/
    // (11)(15)(18)-----(22)(14)(10)
    mesh_data result;
    result.maybe_indices = std::vector<std::uint32_t>{
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        8, 9, 10, 8, 10, 11,
        12, 14, 13, 12, 15, 14,
        17, 16, 19, 17, 19, 18,
        21, 23, 20, 21, 22, 23};
    vec3f half = size / 2.F;

    auto &vertices = result.vertices;
    vertices.reserve(24);
    // Top face vertices (normal: +Y)
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, half.y, half.z}, vec_up, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, half.y, half.z}, vec_up, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, half.y, -half.z}, vec_up, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, half.y, -half.z}, vec_up, vec2f{});

    // Front face vertices (normal: +Z)
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, half.y, half.z}, vec_forward, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, half.y, half.z}, vec_forward, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, -half.y, half.z}, vec_forward, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, -half.y, half.z}, vec_forward, vec2f{});

    // Bottom face vertices (normal: -Y)
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, -half.y, half.z}, vec_down, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, -half.y, half.z}, vec_down, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, -half.y, -half.z}, vec_down, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, -half.y, -half.z}, vec_down, vec2f{});

    // Back face vertices (normal: -Z)
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, half.y, -half.z}, vec_back, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, half.y, -half.z}, vec_back, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, -half.y, -half.z}, vec_back, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, -half.y, -half.z}, vec_back, vec2f{});

    // Left face vertices (normal: -X)
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, half.y, half.z}, vec_left, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, half.y, -half.z}, vec_left, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, -half.y, -half.z}, vec_left, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{-half.x, -half.y, half.z}, vec_left, vec2f{});

    // Right face vertices (normal: +X)
    vertices.emplace_back(color_or_random(color), vec3f{half.x, half.y, half.z}, vec_right, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, half.y, -half.z}, vec_right, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, -half.y, -half.z}, vec_right, vec2f{});
    vertices.emplace_back(color_or_random(color), vec3f{half.x, -half.y, half.z}, vec_right, vec2f{});

    return result;
    // NOLINTEND(*-magic-numbers)
}
} // namespace st
module;

#include <cstdint>
#include <optional>
#include <vector>

module stay3.graphics;

import stay3.core;
import stay3.ecs;
import :material;

namespace st {

vec4f color_or_random(const std::optional<vec4f> &color) {
    return color.has_value() ? *(color) : random_color();
}

mesh_data mesh_plane(const vec2f &size, const std::optional<vec4f> &color, const std::optional<rectf> &rect_normalized) {
    mesh_data result;
    // 01
    // 32
    result.vertices.reserve(4);
    result.maybe_indices = std::vector<std::uint32_t>{0, 2, 1, 0, 3, 2};

    std::array<vec2f, 4> uvs;
    if(rect_normalized.has_value()) {
        const auto &[uv_pos, uv_size] = *rect_normalized;
        uvs = {
            uv_pos,
            uv_pos + vec2f{uv_size.x, 0.F},
            uv_pos + uv_size,
            uv_pos + vec2f{0.F, uv_size.y},
        };
    } else {
        uvs = {
            vec2f{0.F},
            vec2f{1.F, 0.F},
            vec2f{1.F},
            vec2f{0.F, 1.F},
        };
    }

    result.vertices.emplace_back(color_or_random(color), vec3f{-size.x / 2, size.y / 2, 0.F}, vec_back, uvs[0]);
    result.vertices.emplace_back(color_or_random(color), vec3f{size.x / 2, size.y / 2, 0.F}, vec_back, uvs[1]);
    result.vertices.emplace_back(color_or_random(color), vec3f{size.x / 2, -size.y / 2, 0.F}, vec_back, uvs[2]);
    result.vertices.emplace_back(color_or_random(color), vec3f{-size.x / 2, -size.y / 2, 0.F}, vec_back, uvs[3]);

    return result;
}

mesh_data mesh_sprite(const texture_2d_data &texture, float pixels_per_unit, const std::optional<vec4f> &color, const std::optional<rectf> &texture_rect) {
    if(!texture_rect.has_value()) {
        const auto full_size = vec2f{texture.size()} / pixels_per_unit;
        return mesh_plane(full_size, color);
    }
    const auto &[rect_position, rect_size] = *texture_rect;
    const rectf rect_normalized{
        .position = rect_position / vec2f{texture.size()},
        .size = rect_size / vec2f{texture.size()},
    };
    return mesh_plane(rect_size / pixels_per_unit, color, rect_normalized);
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
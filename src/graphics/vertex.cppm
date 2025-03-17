module;

#include <cstdint>
#include <optional>
#include <vector>

export module stay3.graphics:vertex;

import stay3.core;
import stay3.ecs;

export namespace st {
struct vertex_attributes {
    vec4f color;
    vec3f position;
    vec3f normal;
    vec2f uv;
};

struct mesh_data {
    std::vector<vertex_attributes> vertices;
    std::optional<std::vector<std::uint32_t>> maybe_indices;
};

mesh_data mesh_plane(const vec2f &size, const vec4f &color) {
    mesh_data result;
    // 01
    // 32
    result.vertices.reserve(4);
    result.maybe_indices = std::vector<std::uint32_t>{0, 2, 1, 0, 3, 2};

    result.vertices.emplace_back(color, vec3f{-size.x / 2, size.y / 2, 0.F}, vec_back, vec2f{0.F, 0.F});
    result.vertices.emplace_back(color, vec3f{size.x / 2, size.y / 2, 0.F}, vec_back, vec2f{1.F, 0.F});
    result.vertices.emplace_back(color, vec3f{size.x / 2, -size.y / 2, 0.F}, vec_back, vec2f{1.F, 1.F});
    result.vertices.emplace_back(color, vec3f{-size.x / 2, -size.y / 2, 0.F}, vec_back, vec2f{0.F, 1.F});

    return result;
}
} // namespace st
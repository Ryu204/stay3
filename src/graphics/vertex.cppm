module;

#include <cstdint>
#include <optional>
#include <vector>

export module stay3.graphics:vertex;

import stay3.core;

export namespace st {
struct vertex_attributes {
    vec4f color;
    vec3f position;
    vec3f normal;
    vec2f uv;
};

/**
 * @brief Contains geometry definition
 */
struct mesh_data {
    std::vector<vertex_attributes> vertices;
    std::optional<std::vector<std::uint32_t>> maybe_indices;
};

mesh_data mesh_plane(
    const vec2f &size,
    const std::optional<vec4f> &color = std::nullopt,
    const std::optional<rectf> &rect_normalized = std::nullopt);
class texture_2d_data;
mesh_data mesh_sprite(
    const texture_2d_data &texture,
    float pixels_per_unit,
    const std::optional<vec4f> &color = std::nullopt,
    const std::optional<rectf> &texture_rect = std::nullopt);
mesh_data mesh_cube(const vec3f &size, const std::optional<vec4f> &color = std::nullopt);
} // namespace st
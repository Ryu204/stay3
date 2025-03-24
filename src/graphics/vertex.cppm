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

/**
 * @brief Contains geometry definition
 */
struct mesh_data {
    std::vector<vertex_attributes> vertices;
    std::optional<std::vector<std::uint32_t>> maybe_indices;
};

mesh_data mesh_plane(const vec2f &size, const std::optional<vec4f> &color = std::nullopt);
mesh_data mesh_cube(const vec3f &size, const std::optional<vec4f> &color = std::nullopt);

/**
 * @brief This component represent single visible object in the scene
 */
struct rendered_mesh {
    entity mesh_holder;
    entity material_holder;
};
} // namespace st
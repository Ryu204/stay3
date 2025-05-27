module;

#include <cstdint>
#include <optional>
#include <vector>

export module stay3.graphics.core:vertex;

import stay3.core;

export namespace st {
struct vertex_attributes {
    vec4f color{1.F};
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
} // namespace st
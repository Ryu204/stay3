module;

#include <cassert>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

export module stay3.graphics.core:mesh_builder;

import stay3.core;
import stay3.ecs;

import :vertex;
import :material;

export namespace st {

struct mesh_plane_builder {
    vec2f size;
    vec2f origin{0.5F};
    vec4f color{1.F};
    std::optional<rectf> uv_rect{std::nullopt};
    [[nodiscard]] mesh_data build() const {
        mesh_data result;
        // 01
        // 32
        result.maybe_indices = {0, 2, 1, 0, 3, 2};

        std::array<vec2f, 4> uvs;
        if(uv_rect.has_value()) {
            auto &&[uv_pos, uv_size] = *uv_rect;
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

        auto &vertices = result.vertices;
        vertices.reserve(4);
        vertices.emplace_back(color, vec3f{}, vec_back, uvs[0]);
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_back, uvs[1]);
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_back, uvs[2]);
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_back, uvs[3]);

        const vec3f offset{-size.x * origin.x, size.y * origin.y, 0.F};
        for(auto &vert: vertices) {
            vert.position += offset;
        }

        return result;
    }
};

struct mesh_sprite_builder {
public:
    component_ref<texture_2d> texture;
    float pixels_per_unit{};
    vec2f origin{0.5F};
    vec4f color{1.F};
    std::optional<rectf> texture_rect{std::nullopt};

    [[nodiscard]] mesh_data build(ecs_registry &reg) const {
        assert(!texture.is_null() && "Unset texture");
        const auto texture = this->texture.get(reg);
        if(!texture_rect.has_value()) {
            const auto full_size = vec2f{texture->size()} / pixels_per_unit;
            return mesh_plane_builder{
                .size = full_size,
                .origin = origin,
                .color = color,
            }
                .build();
        }
        auto &&[rect_position, rect_size] = *texture_rect;
        const rectf uv_rect{
            .position = rect_position / vec2f{texture->size()},
            .size = rect_size / vec2f{texture->size()},
        };
        return mesh_plane_builder{
            .size = rect_size / pixels_per_unit,
            .origin = origin,
            .color = color,
            .uv_rect = uv_rect,
        }
            .build();
    }
};

struct mesh_cube_builder {
    vec3f size;
    vec3f origin{0.5F};
    vec4f color{1.F};

    [[nodiscard]] mesh_data build() const {
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
        vertices.emplace_back(color, vec3f{0.F, 0.F, size.z}, vec_up, vec2f{0.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, 0.F, size.z}, vec_up, vec2f{1.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_up, vec2f{1.F, 0.F});
        vertices.emplace_back(color, vec3f{0.F, 0.F, 0.F}, vec_up, vec2f{0.F, 0.F});

        // Front face vertices (normal: +Z)
        vertices.emplace_back(color, vec3f{0.F, 0.F, size.z}, vec_forward, vec2f{0.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, 0.F, size.z}, vec_forward, vec2f{1.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, -size.y, size.z}, vec_forward, vec2f{1.F, 0.F});
        vertices.emplace_back(color, vec3f{0.F, -size.y, size.z}, vec_forward, vec2f{0.F, 0.F});

        // Bottom face vertices (normal: -Y)
        vertices.emplace_back(color, vec3f{0.F, -size.y, size.z}, vec_down, vec2f{0.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, -size.y, size.z}, vec_down, vec2f{1.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_down, vec2f{1.F, 0.F});
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_down, vec2f{0.F, 0.F});

        // Back face vertices (normal: -Z)
        vertices.emplace_back(color, vec3f{0.F, 0.F, 0.F}, vec_back, vec2f{0.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_back, vec2f{1.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_back, vec2f{1.F, 0.F});
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_back, vec2f{0.F, 0.F});

        // Left face vertices (normal: -X)
        vertices.emplace_back(color, vec3f{0.F, 0.F, size.z}, vec_left, vec2f{1.F, 1.F});
        vertices.emplace_back(color, vec3f{0.F, 0.F, 0.F}, vec_left, vec2f{0.F, 1.F});
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_left, vec2f{0.F, 0.F});
        vertices.emplace_back(color, vec3f{0.F, -size.y, size.z}, vec_left, vec2f{1.F, 0.F});

        // Right face vertices (normal: +X)
        vertices.emplace_back(color, vec3f{size.x, 0.F, size.z}, vec_right, vec2f{1.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_right, vec2f{0.F, 1.F});
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_right, vec2f{0.F, 0.F});
        vertices.emplace_back(color, vec3f{size.x, -size.y, size.z}, vec_right, vec2f{1.F, 0.F});

        const vec3f offset{-size.x * origin.x, size.y * origin.y, -size.z * origin.z};
        for(auto &vert: vertices) {
            vert.position += offset;
        }

        return result;
        // NOLINTEND(*-magic-numbers)
    }
};

struct mesh_uv_sphere_builder {
    float radius{1.F};
    vec3f origin{0.5F};
    vec4f color{1.F};
    std::uint32_t segments{32};
    std::uint32_t rings{16};

    /**
     * @brief How the UVs look like with `(seg, ring) = (7, 5)`:
     *  /\/\/\/\/\/\/\
     * | | | | | | | |
     * | | | | | | | |
     * | | | | | | | |
     * \/\/\/\/\/\/\/
     */
    [[nodiscard]] mesh_data build() const {
        assert(rings >= 3 && segments >= 3 && "Insufficient dimensions");
        mesh_data result;

        auto &vertices = result.vertices;
        auto &indices = result.maybe_indices.emplace();
        vertices.reserve(((1ull + segments) * (rings - 1)) + (2ull * segments));
        // NOLINTBEGIN(*-identifier-length)
        // North pole
        for(auto seg = 0; seg < segments; ++seg) {
            const auto u = (static_cast<float>(seg) + 0.5F) / static_cast<float>(segments);
            vertices.emplace_back(color, radius * vec_up, vec_up, vec2f{u, 0.F});
        }
        for(auto ring = 1; ring < rings; ++ring) {
            const auto v = static_cast<float>(ring) / static_cast<float>(rings);
            const auto theta = PI * v;
            vec3f normal;
            normal.y = std::cos(theta);
            const auto sin_theta = std::sin(theta);
            for(auto seg = 0; seg <= segments; ++seg) {
                const auto u = static_cast<float>(seg) / static_cast<float>(segments);
                const auto phi = 2 * PI * u;
                normal.z = sin_theta * std::sin(phi);
                normal.x = sin_theta * std::cos(phi);
                vertices.emplace_back(color, radius * normal, normal, vec2f{u, v});
            }
        }
        // South pole
        for(auto seg = 0; seg < segments; ++seg) {
            const auto u = (static_cast<float>(seg) + 0.5F) / static_cast<float>(segments);
            vertices.emplace_back(color, radius * vec_down, vec_down, vec2f{u, 1.F});
        }
        // NOLINTEND(*-identifier-length)
        static constexpr auto indices_per_trig = 3;
        indices.reserve(1ull * rings * segments * 2 * indices_per_trig);
        auto least_index = 0;
        for(auto ring = 0; ring < rings; ++ring) {
            if(ring == 0) {
                for(auto seg = 0; seg < segments; ++seg) {
                    indices.push_back(least_index);
                    indices.push_back(least_index + segments);
                    indices.push_back(least_index + segments + 1);
                    ++least_index;
                }
            } else if(ring + 1 == rings) {
                for(auto seg = 0; seg < segments; ++seg) {
                    indices.push_back(least_index);
                    indices.push_back(least_index + segments + 1);
                    indices.push_back(least_index + 1);
                    ++least_index;
                }
            } else {
                for(auto seg = 0; seg < segments; ++seg) {
                    indices.push_back(least_index);
                    indices.push_back(least_index + segments + 1);
                    indices.push_back(least_index + 1);
                    indices.push_back(least_index + 1);
                    indices.push_back(least_index + segments + 1);
                    indices.push_back(least_index + segments + 2);
                    ++least_index;
                }
                ++least_index;
            }
        }
        return result;
    }
};

} // namespace st
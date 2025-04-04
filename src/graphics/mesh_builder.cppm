module;

#include <cassert>
#include <concepts>
#include <cstdint>
#include <optional>
#include <vector>

export module stay3.graphics:mesh_builder;

import stay3.core;

import :vertex;
import :material;

export namespace st {

template<typename builder>
concept mesh_builder = requires(const builder &bd) {
    { bd.build() } -> std::convertible_to<mesh_data>;
};

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
static_assert(mesh_builder<mesh_plane_builder>);

struct mesh_sprite_builder {
public:
    const texture_2d_data *texture{};
    float pixels_per_unit{};
    vec2f origin{0.5F};
    vec4f color{1.F};
    std::optional<rectf> texture_rect{std::nullopt};

    [[nodiscard]] mesh_data build() const {
        assert(texture != nullptr && "Unset texture");
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
static_assert(mesh_builder<mesh_sprite_builder>);

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
        vertices.emplace_back(color, vec3f{0.F, 0.F, size.z}, vec_up, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, 0.F, size.z}, vec_up, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_up, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, 0.F, 0.F}, vec_up, vec2f{});

        // Front face vertices (normal: +Z)
        vertices.emplace_back(color, vec3f{0.F, 0.F, size.z}, vec_forward, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, 0.F, size.z}, vec_forward, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, -size.y, size.z}, vec_forward, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, -size.y, size.z}, vec_forward, vec2f{});

        // Bottom face vertices (normal: -Y)
        vertices.emplace_back(color, vec3f{0.F, -size.y, size.z}, vec_down, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, -size.y, size.z}, vec_down, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_down, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_down, vec2f{});

        // Back face vertices (normal: -Z)
        vertices.emplace_back(color, vec3f{0.F, 0.F, 0.F}, vec_back, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_back, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_back, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_back, vec2f{});

        // Left face vertices (normal: -X)
        vertices.emplace_back(color, vec3f{0.F, 0.F, size.z}, vec_left, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, 0.F, 0.F}, vec_left, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, -size.y, 0.F}, vec_left, vec2f{});
        vertices.emplace_back(color, vec3f{0.F, -size.y, size.z}, vec_left, vec2f{});

        // Right face vertices (normal: +X)
        vertices.emplace_back(color, vec3f{size.x, 0.F, size.z}, vec_right, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, 0.F, 0.F}, vec_right, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, -size.y, 0.F}, vec_right, vec2f{});
        vertices.emplace_back(color, vec3f{size.x, -size.y, size.z}, vec_right, vec2f{});

        const vec3f offset{-size.x * origin.x, size.y * origin.y, -size.z * origin.z};
        for(auto &vert: vertices) {
            vert.position += offset;
        }

        return result;
        // NOLINTEND(*-magic-numbers)
    }
};
static_assert(mesh_builder<mesh_cube_builder>);
} // namespace st
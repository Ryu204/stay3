#include <catch2/catch_all.hpp>
import stay3.graphics;
import stay3.core;
import stay3.ecs;

using namespace st;

constexpr vec4f red{1.F, 0.F, 0.F, 1.F};

TEST_CASE("Plane") {
    SECTION("No uv rect") {
        mesh_plane_builder builder{
            .size = {2.F, 2.F},
            .color = red,
            .uv_rect = std::nullopt,
        };

        mesh_data mesh = builder.build();
        SECTION("Size") {
            REQUIRE(mesh.vertices.size() == 4);
            REQUIRE(mesh.maybe_indices.has_value());
            REQUIRE(mesh.maybe_indices->size() == 6);
        }
        SECTION("Position") {
            for(const auto &vertex: mesh.vertices) {
                REQUIRE(vertex.position.z == 0.F);
            }
            REQUIRE(mesh.vertices[0].position.x == Catch::Approx{-1.F});
            REQUIRE(mesh.vertices[0].position.y == Catch::Approx{1.F});

            REQUIRE(mesh.vertices[2].position.x == Catch::Approx{1.F});
            REQUIRE(mesh.vertices[2].position.y == Catch::Approx{-1.F});
        }
        SECTION("uv") {
            REQUIRE(mesh.vertices[0].uv.x == 0.F);
            REQUIRE(mesh.vertices[0].uv.y == 0.F);
            REQUIRE(mesh.vertices[2].uv.x == 1.F);
            REQUIRE(mesh.vertices[2].uv.y == 1.F);
        }
        SECTION("Color") {
            for(const auto &vertex: mesh.vertices) {
                REQUIRE(vertex.color == red);
            }
        }
        SECTION("Normal") {
            for(const auto &vertex: mesh.vertices) {
                REQUIRE(vertex.normal == vec_back);
            }
        }
    }

    SECTION("uv rect and origin") {
        mesh_plane_builder builder{
            .size = {2.F, 2.F},
            .origin = {0.F, 0.F},
            .color = red,
            .uv_rect = rectf{.position = {0.4F, 0.6F}, .size = {0.2F, 0.4F}},
        };

        mesh_data mesh = builder.build();

        REQUIRE(mesh.vertices[0].uv.x == Catch::Approx{0.4F});
        REQUIRE(mesh.vertices[0].uv.y == Catch::Approx{0.6F});

        REQUIRE(mesh.vertices[2].uv.x == Catch::Approx{0.6F});
        REQUIRE(mesh.vertices[2].uv.y == Catch::Approx{1.F});
    }
}

TEST_CASE("Sprite") {
    ecs_registry reg;
    entity en = reg.create();

    constexpr auto pixels_per_unit = 64.F;
    constexpr vec2u texture_size{1920, 1080};
    const auto &texture = *reg.emplace<texture_2d>(en, texture_2d::format::rgba8unorm, texture_size);
    const vec2f size = vec2f{texture.size()} / pixels_per_unit;

    constexpr rectf texture_rect{.position = {1000, 1200}, .size = {20, 50}};
    const rectf normed_texture_rect{
        .position = vec2f{1000, 1200} / vec2f{texture.size()},
        .size = vec2f{20, 50} / vec2f{texture.size()},
    };

    SECTION("Full texture") {
        mesh_sprite_builder builder{
            .texture = en,
            .pixels_per_unit = pixels_per_unit,
            .color = red,
            .texture_rect = std::nullopt,
        };

        const auto mesh = builder.build(reg);

        SECTION("Size") {
            REQUIRE(mesh.vertices.size() == 4);
            REQUIRE(mesh.maybe_indices.has_value());
            REQUIRE(mesh.maybe_indices->size() == 6);
        }
        SECTION("Position") {
            for(const auto &vertex: mesh.vertices) {
                REQUIRE(vertex.position.z == 0.F);
            }

            REQUIRE(mesh.vertices[0].position.x == Catch::Approx{-size.x / 2.F});
            REQUIRE(mesh.vertices[0].position.y == Catch::Approx{size.y / 2.F});

            REQUIRE(mesh.vertices[2].position.x == Catch::Approx{size.x / 2.F});
            REQUIRE(mesh.vertices[2].position.y == Catch::Approx{-size.y / 2.F});
        }
        SECTION("uv") {
            REQUIRE(mesh.vertices[0].uv.x == 0.F);
            REQUIRE(mesh.vertices[0].uv.y == 0.F);
            REQUIRE(mesh.vertices[2].uv.x == 1.F);
            REQUIRE(mesh.vertices[2].uv.y == 1.F);
        }
        SECTION("Color") {
            for(const auto &vertex: mesh.vertices) {
                REQUIRE(vertex.color == red);
            }
        }
        SECTION("Normal") {
            for(const auto &vertex: mesh.vertices) {
                REQUIRE(vertex.normal == vec_back);
            }
        }
    }

    SECTION("With texture rect") {
        mesh_sprite_builder builder{
            .texture = en,
            .pixels_per_unit = pixels_per_unit,
            .color = red,
            .texture_rect = texture_rect,
        };

        mesh_data mesh = builder.build(reg);

        REQUIRE(mesh.vertices[0].uv.x == Catch::Approx{normed_texture_rect.left()});
        REQUIRE(mesh.vertices[0].uv.y == Catch::Approx{normed_texture_rect.top()});

        REQUIRE(mesh.vertices[2].uv.x == Catch::Approx{normed_texture_rect.right()});
        REQUIRE(mesh.vertices[2].uv.y == Catch::Approx{normed_texture_rect.bottom()});
    }
}

TEST_CASE("mesh_cube_builder builds correct cubes") {
    mesh_cube_builder builder{
        .size = {2.F, 2.F, 2.F},
        .color = red,
    };

    mesh_data mesh = builder.build();
    SECTION("Data size") {
        REQUIRE(mesh.vertices.size() == 24);
        REQUIRE(mesh.maybe_indices.has_value());
        REQUIRE(mesh.maybe_indices->size() == 36);
    }

    SECTION("Position") {
        REQUIRE(mesh.vertices[0].position.x == Catch::Approx{-1.F});
        REQUIRE(mesh.vertices[0].position.y == Catch::Approx{1.F});
        REQUIRE(mesh.vertices[0].position.z == Catch::Approx{1.F});

        REQUIRE(mesh.vertices[1].position.x == Catch::Approx{1.F});
        REQUIRE(mesh.vertices[1].position.y == Catch::Approx{1.F});
        REQUIRE(mesh.vertices[1].position.z == Catch::Approx{1.F});

        REQUIRE(mesh.vertices[10].position.x == Catch::Approx{1.F});
        REQUIRE(mesh.vertices[10].position.y == Catch::Approx{-1.F});
        REQUIRE(mesh.vertices[10].position.z == Catch::Approx{-1.F});
    }
    SECTION("Normal") {
        REQUIRE(mesh.vertices[0].normal == vec_up);
        REQUIRE(mesh.vertices[4].normal == vec_forward);
        REQUIRE(mesh.vertices[8].normal == vec_down);
        REQUIRE(mesh.vertices[12].normal == vec_back);
        REQUIRE(mesh.vertices[16].normal == vec_left);
        REQUIRE(mesh.vertices[20].normal == vec_right);
    }
    SECTION("Color") {
        for(const auto &vertex: mesh.vertices) {
            REQUIRE(vertex.color == red);
        }
    }
}

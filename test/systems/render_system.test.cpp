#include <cmath>
#include <catch2/catch_all.hpp>
import stay3;
using namespace st;

TEST_CASE("Render some entity") {
    struct my_render_system {
        static void start(tree_context &ctx) {
            auto &reg = ctx.ecs();
            auto &texture_cmd = ctx.vars().get<texture_2d::commands>();
            auto entity1 = ctx.root().entities().create();
            auto entity2 = ctx.root().entities().create();
            auto entity3 = ctx.root().entities().create();
            auto material_en = ctx.root().entities().create();
            auto cam = ctx.root().entities().create();

            texture_cmd.emplace(texture_2d::command_load{
                .target = material_en,
                .filename = "assets/texture_checker.jpg",
            });
            reg.emplace<material>(material_en, material{.texture = material_en});

            reg.emplace<mesh_plane_builder>(
                entity1,
                mesh_plane_builder{
                    .size = {1, 2},
                    .color = {1, 0, 1, 1},
                });
            reg.emplace<rendered_mesh>(
                entity1,
                rendered_mesh{
                    .mesh = entity1,
                    .mat = material_en,
                });

            reg.emplace<mesh_sprite_builder>(
                entity2,
                mesh_sprite_builder{
                    .texture = material_en,
                    .pixels_per_unit = 100.F,
                    .color = {1, 0, 1, 1},
                });
            reg.emplace<rendered_mesh>(
                entity2,
                rendered_mesh{
                    .mesh = entity2,
                    .mat = material_en,
                });

            reg.emplace<mesh_cube_builder>(
                entity3,
                mesh_cube_builder{
                    .size = {1, 2, 3},
                    .color = {1, 0, 1, 1},
                });
            reg.emplace<rendered_mesh>(
                entity3,
                rendered_mesh{
                    .mesh = entity3,
                    .mat = material_en,
                });

            reg.emplace<main_camera>(cam);
            reg.emplace<camera>(cam);
            reg.get<mut<transform>>(cam)->translate(5.F * vec_back);
        }
        sys_run_result update(seconds delta, tree_context &ctx) {
            auto &reg = ctx.ecs();
            for(auto [en, builder]: reg.each<mut<mesh_plane_builder>>()) {
                builder->size = vec2f{std::sin(elapsed_time)};
            }

            elapsed_time += delta;
            if(elapsed_time >= 5.F && rendered) {
                return sys_run_result::exit;
            }
            return sys_run_result::noop;
        }
        void render(tree_context &) {
            rendered = true;
        }

        bool rendered{false};
        seconds elapsed_time{};
    };

    app my_app;
    my_app
        .systems()
        .add<my_render_system>()
        .run_as<sys_type::start>(sys_priority::very_low)
        .run_as<sys_type::update>()
        .run_as<sys_type::render>();
    REQUIRE_NOTHROW(my_app.run());
}
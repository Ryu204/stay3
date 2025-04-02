#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
import stay3;

using namespace st;

struct setup_system {
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();

        auto &res = ctx.root().add_child();
        auto &scene = ctx.root().add_child();

        mesh1 = res.entities().create();
        mesh2 = res.entities().create();
        auto entity1 = scene.entities().create();
        auto entity2 = scene.entities().create();
        auto cam = scene.entities().create();

        reg.emplace<mesh_data>(mesh2, mesh_cube(vec3f{1.5F, 0.2F, 1.F}));
        reg.emplace<mesh_data>(mesh1, mesh_plane(vec2f{1.F, 1.F}, vec4f{1.F}));
        auto texture1 = reg.emplace<texture_2d_data>(mesh1, "assets/textures/example.jpg");
        auto material1 = reg.emplace<material_data>(mesh1, material_data{.texture = mesh1});
        auto material2 = reg.emplace<material_data>(mesh2);

        reg.emplace<rendered_mesh>(
            entity1,
            rendered_mesh{
                .mesh = mesh1,
                .material = mesh1,
            });
        {
            auto tf = reg.get<mut<transform>>(entity1);
            tf->scale(5.F);
        }
        reg.emplace<rendered_mesh>(
            entity2,
            rendered_mesh{
                .mesh = mesh2,
                .material = mesh2,
            });

        reg.emplace<camera>(cam);
        reg.emplace<main_camera>(cam);
        auto tf = reg.get<mut<transform>>(cam);
        tf->set_position(vec_back * 3.F + 1.5F * vec_up);
    }

    void update(seconds delta, tree_context &ctx) {
        total_elapsed += delta;
        auto &reg = ctx.ecs();
        for(auto [en, mesh, tf]: reg.each<rendered_mesh, mut<transform>>()) {
            tf->set_orientation(vec_right, (PI / 2.F) + (std::cos(total_elapsed + 4.F) / 3.F));
            tf->set_scale(2.F + std::sin(total_elapsed));
        }
    }

    entity mesh1;
    entity mesh2;
    seconds total_elapsed{};
};

int main() {
    constexpr vec2u win_size{600u, 400u};
    constexpr auto updates_per_sec = 20;
    try {
        app my_app{{
            .window = {
                .size = win_size,
                .name = "My cool window",
            },
            .updates_per_second = updates_per_sec,
            .render = {.power_pref = render_config::power_preference::low},
        }};
        my_app
            .systems()
            .add<setup_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

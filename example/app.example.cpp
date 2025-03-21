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

        reg.add_component<const mesh_data>(mesh2, mesh_cube(vec3f{1.5F, 0.2F, 1.F}));
        reg.add_component<const mesh_data>(mesh1, mesh_plane(vec2f{1.F, 1.F}, vec4f{0.05F, 0.6F, 0.8F, 1.F}));

        reg.add_component<const rendered_mesh>(entity1, mesh2);
        reg.add_component<const rendered_mesh>(entity2, mesh1);

        reg.add_component<const camera>(cam);
        reg.add_component<const main_camera>(cam);
        auto tf = reg.get_components<transform>(cam);
        tf->set_position(vec_back * 3.F + vec_up);
    }

    void update(seconds delta, tree_context &ctx) {
        total_elapsed += delta;
        auto &reg = ctx.ecs();
        for(auto [en, mesh, tf]: reg.each<const rendered_mesh, transform>()) {
            tf->rotate(vec_back, delta);
        }
        auto m1 = reg.get_components<mesh_data>(mesh1);
        auto m2 = reg.get_components<mesh_data>(mesh2);
        *m1 = mesh_plane(vec2f{std::max<float>(0.1F, std::fmod(total_elapsed, 1.F))});
        *m2 = mesh_cube(vec3f{std::max<float>(0.1F, std::fmod((total_elapsed / 10.F) + 2.345345F, 1.F))});
    }

    entity mesh1;
    entity mesh2;
    seconds total_elapsed{};
};

int main() {
    try {
        app my_app{
            {
                .window = {
                    .size = {600u, 400u},
                    .name = "My cool window",
                },
                .updates_per_second = 20,
                .render = {.power_pref = render_config::power_preference::low},
            },
        };
        my_app.enable_default_systems();
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

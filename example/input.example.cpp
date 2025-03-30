#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
import stay3;

using namespace st;

struct input_system {
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();

        auto &res = ctx.root().add_child();
        auto &scene = ctx.root().add_child();

        mesh_holders = {
            res.entities().create(),
            res.entities().create(),
        };
        auto material = res.entities().create();

        auto en = scene.entities().create();
        auto cam = scene.entities().create();

        reg.add_component<const mesh_data>(mesh_holders[0], mesh_plane(vec2f{1.F, 1.F}, vec4f{1.F}));
        reg.add_component<const mesh_data>(mesh_holders[1], mesh_plane(vec2f{0.5F, 2.F}));
        reg.add_component<const material_data>(material);
        reg.add_component<const rendered_mesh>(
            en,
            rendered_mesh{
                .mesh_holder = mesh_holders[0],
                .material_holder = material,
            });

        reg.add_component<const camera>(cam);
        reg.add_component<const main_camera>(cam);
        auto tf = reg.get_components<transform>(cam);
        tf->set_position(vec_back * 2.F);
    }

    void input(const event &ev, tree_context &ctx) const {
        if(const auto *data = ev.try_get<event::key_pressed>(); data != nullptr) {
            std::cout << get_name(data->code) << std::endl; // NOLINT(*-avoid-endl)
        }
        auto &window = ctx.ecs().get_context<runtime_info>().window();

        entity current_mesh;
        if(window.get_key(scancode::space) == key_status::pressed) {
            current_mesh = mesh_holders[1];
        } else {
            current_mesh = mesh_holders[0];
        }
        for(auto [unused, data]: ctx.ecs().each<rendered_mesh>()) {
            data->mesh_holder = current_mesh;
        }
    }

    std::array<entity, 2> mesh_holders;
};

int main() {
    try {
        app my_app;
        my_app.enable_default_systems();
        my_app
            .systems()
            .add<input_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::input>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

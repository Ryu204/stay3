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
        auto material_en = res.entities().create();

        auto en = scene.entities().create();
        auto cam = scene.entities().create();

        reg.emplace<mesh_plane_builder>(mesh_holders[0], mesh_plane_builder{.size = vec2f{1.F}});
        reg.emplace<mesh_plane_builder>(mesh_holders[1], mesh_plane_builder{.size = vec2f{0.5F, 2.F}});
        reg.emplace<material>(material_en);
        reg.emplace<rendered_mesh>(
            en,
            rendered_mesh{
                .mesh = mesh_holders[0],
                .mat = material_en,
            });

        reg.emplace<camera>(cam);
        reg.emplace<main_camera>(cam);
        auto tf = reg.get<mut<transform>>(cam);
        tf->set_position(vec_back * 2.F);
    }

    void input(const event &ev, tree_context &ctx) const {
        if(const auto *data = ev.try_get<event::key_pressed>(); data != nullptr) {
            std::cout << get_name(data->code) << std::endl; // NOLINT(*-avoid-endl)
        }
        auto &window = ctx.vars().get<runtime_info>().window();

        entity current_mesh;
        if(window.get_key(scancode::space) == key_status::pressed) {
            current_mesh = mesh_holders[1];
        } else {
            current_mesh = mesh_holders[0];
        }
        for(auto [unused, data]: ctx.ecs().each<mut<rendered_mesh>>()) {
            data->mesh = current_mesh;
        }
    }

    std::array<entity, 2> mesh_holders;
};

int main() {
    try {
        app_launcher my_app;
        my_app
            .systems()
            .add<input_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::input>();
        my_app.launch();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

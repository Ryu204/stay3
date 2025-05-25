#include <cassert>
#include <exception>
#include <iostream>

import stay3;

using namespace st;

using name = std::string;
struct red_box {};

struct example_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        constexpr vec3f box_size_1{1, 2, 3};
        constexpr vec3f box_size_2{2, 2, 1};
        constexpr vec3f floor_size{10.F, 0.1F, 10.F};
        auto material_en_1 = ctx.root().entities().create();
        auto material_en_2 = ctx.root().entities().create();
        reg.emplace<material>(material_en_1, material{.color = {1.F, 0.F, 0.F, 0.5F}});
        reg.emplace<material>(material_en_2, material{.color = {0.5F, 1.F, 0.7F, 0.7F}});

        auto box_en_1 = create_box(reg, ctx.root(), material_en_1, box_size_1, rigidbody::type::dynamic);
        reg.get<mut<transform>>(box_en_1)
            ->translate(vec_up * 25.F)
            .rotate(vec_back, PI / 6.F)
            .rotate(vec_right, PI / 3.F);
        reg.emplace<name>(box_en_1, "red box");
        reg.emplace<red_box>(box_en_1);
        auto box_en_2 = create_box(reg, ctx.root(), material_en_2, box_size_2, rigidbody::type::dynamic);
        reg.get<mut<transform>>(box_en_2)
            ->translate(vec_up * 3.F + vec_right * 3.F)
            .rotate(vec_up, PI / 6.F)
            .rotate(vec_forward, PI / 3.F);
        reg.emplace<name>(box_en_2, "green box");
        auto floor_en = create_box(reg, ctx.root(), material_en_2, floor_size, rigidbody::type::fixed);
        reg.get<mut<transform>>(floor_en)->translate(vec_down * 3.F);
        reg.emplace<name>(floor_en, "floor");

        // Camera setup
        auto cam_en = ctx.root().entities().create();
        reg.emplace<camera>(cam_en, camera{.data = camera::perspective_data{}});
        reg.emplace<main_camera>(cam_en);
        reg.get<mut<transform>>(cam_en)->translate(vec_back * 10.F + vec_up * 2.F);
    }

    static sys_run_result input(const event &ev, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(auto *key = ev.try_get<event::key_pressed>(); key != nullptr) {
            if(key->code == scancode::esc) {
                return sys_run_result::exit;
            } else if(key->code == scancode::space) {
                reg.get<mut<motion>>(reg.view<red_box>().front())->set_linear_velocity(vec_up * 10.F);
            }
        }
        return sys_run_result::noop;
    }

    static void update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, rg, en_name]: reg.each<rigidbody, name>()) {
            for(const auto &enter: *reg.get<collision_enter>(en)) {
                log::info(*reg.get<name>(enter.other), " began collision with ", *en_name);
            }
            for(const auto &exit_en: *reg.get<collision_exit>(en)) {
                log::info(*reg.get<name>(exit_en), " exited collision with ", *en_name);
            }
        }
    }

private:
    static entity create_box(ecs_registry &reg, node &holder, entity material_holder, const vec3f &box_size, rigidbody::type type) {
        auto box_en = holder.entities().create();
        // Visual
        reg.emplace<mesh_cube_builder>(box_en, mesh_cube_builder{.size = box_size + 0.05F});
        reg.emplace<rendered_mesh>(box_en, rendered_mesh{.mesh = box_en, .mat = material_holder});
        // Physics
        reg.emplace<rigidbody>(box_en, rigidbody{.motion_type = type, .allow_sleep = false});
        reg.emplace<collider>(box_en, box_size);
        reg.emplace<collision_enter>(box_en);
        reg.emplace<collision_exit>(box_en);
        return box_en;
    }
};

int main() {
    try {
        app my_app{{
            .window = {.size = {1000, 800}},
            .physics = {.debug_draw = false},
            .assets_dir = "../assets",
        }};
        my_app
            .systems()
            .add<example_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::input>()
            .run_as<sys_type::update>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

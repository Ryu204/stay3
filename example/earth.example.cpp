#include <cmath>
#include <exception>
#include <iostream>
import stay3;
using namespace st;

struct earth_tag {};

struct my_system {
    static constexpr auto tilt_angle = 23.4F / 180.F * PI;
    static inline const vec3f tilt{-std::sin(tilt_angle), std::cos(tilt_angle), 0.F};
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto earth = ctx.root().entities().create();
        reg.emplace<earth_tag>(earth);
        reg.emplace<mesh_uv_sphere_builder>(
            earth,
            mesh_uv_sphere_builder{.radius = 1.5F});
        auto &texture_cmds = ctx.vars().get<texture_2d::commands>();
        texture_cmds.emplace(texture_2d::command_load{
            .target = earth,
            .filename = "assets/2k_earth_daymap.jpg"});
        reg.emplace<material>(earth, material{.texture = earth});
        reg.emplace<rendered_mesh>(earth, rendered_mesh{.mesh = earth, .mat = earth});
        create_default_camera(ctx);
    }
    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto earth: reg.view<earth_tag>()) {
            reg.get<mut<transform>>(earth)->rotate(tilt, delta / 2.F);
        }
    }
    void input(const event &ev, tree_context &ctx) {
        if(is_space_pressed) { return; }
        if(auto *key = ev.try_get<event::key_pressed>(); key == nullptr || key->code != scancode::space) {
            return;
        }
        is_space_pressed = true;
        auto &reg = ctx.ecs();
        reg.get<mut<transform>>(reg.view<main_camera>().front())->translate(vec_back * 8.F);
        auto earth = reg.view<earth_tag>().front();
        reg.emplace<rigidbody>(earth, rigidbody::dynamic);
        reg.emplace<collider>(earth, sphere{.radius = 1.5F});

        // Create a floor
        auto floor = ctx.root().entities().create();
        reg.emplace<mesh_cube_builder>(floor, mesh_cube_builder{.size = {4.F, 0.1F, 4.F}});
        reg.emplace<material>(floor, material{.color = {0.6F, 0.1F, 0.3F, 1.F}});
        reg.emplace<rendered_mesh>(floor, rendered_mesh{.mesh = floor, .mat = floor});
        reg.emplace<rigidbody>(floor, rigidbody::fixed);
        reg.emplace<collider>(floor, box{4.F, 0.1F, 4.F});
        reg.get<mut<transform>>(floor)->translate(vec_down * 5.F).rotate(vec_right, -PI / 4).rotate(vec_up, PI / 4.F);
    }
    static void create_default_camera(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto cam_en = ctx.root().entities().create();
        reg.emplace<camera>(cam_en, camera{.data = camera::perspective_data{}});
        reg.emplace<main_camera>(cam_en);
        reg.get<mut<transform>>(cam_en)->translate(vec_back * 4.F);
    }

    bool is_space_pressed{false};
};

int main() {
    try {
        app my_app{{.assets_dir = "../assets"}};
        my_app.systems()
            .add<my_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>()
            .run_as<sys_type::input>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << e.what() << '\n';
    }
}
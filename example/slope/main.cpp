#include <exception>
#include <iostream>
import stay3;
using namespace st;

namespace data {
constexpr vec3f camera_offset{0.F, 2.F, -7.F};
constexpr vec3f platform_size{6.F, 30.F, 30.F};
constexpr float ball_radius = 1.F;
} // namespace data

struct tags {
    struct ball {};
};

struct camera_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto cam_en = ctx.root().entities().create();
        reg.emplace<main_camera>(cam_en);
        reg.emplace<camera>(cam_en);
        reg.get<mut<transform>>(cam_en)->translate(data::camera_offset);
    }
};

struct ball_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &texture_cmd = ctx.vars().get<texture_2d::commands>();
        auto ball_en = ctx.root().entities().create();
        reg.emplace<mesh_uv_sphere_builder>(ball_en, mesh_uv_sphere_builder{.radius = data::ball_radius});
        texture_cmd.emplace(texture_2d::command_load{.target = ball_en, .filename = "assets/beach_ball.jpg"});
        reg.emplace<material>(ball_en, material{.texture = ball_en});
        reg.emplace<rendered_mesh>(ball_en, rendered_mesh{.mesh = ball_en, .mat = ball_en});
        reg.emplace<tags::ball>(ball_en);
        reg.get<mut<transform>>(ball_en)->translate(4.F * data::ball_radius * vec_up);
    }

    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, tf, ball]: reg.each<mut<transform>, tags::ball>()) {
            tf->rotate(vec_up + vec_left, delta);
        }
    }
};

struct platform_system {
    static void start(tree_context &ctx) {
        auto platform_en = ctx.root().entities().create();
        create_platform(ctx, platform_en);
        auto &reg = ctx.ecs();
        reg.get<mut<transform>>(platform_en)->translate(vec_down * 2.F);
    }

private:
    struct material_tag {};
    static entity default_material_en(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto view = reg.view<material_tag>();
        if(view.begin() != view.end()) {
            return view.front();
        }
        auto mat_en = reg.create();
        auto &texture_cmd = ctx.vars().get<texture_2d::commands>();
        texture_cmd.emplace(texture_2d::command_load{.target = mat_en, .filename = "assets/block.jpg"});
        reg.emplace<material>(mat_en, material{.texture = mat_en});
        reg.emplace<material_tag>(mat_en);
        return mat_en;
    }

    struct mesh_tag {};
    static entity default_mesh_en(ecs_registry &reg) {
        auto view = reg.view<mesh_tag>();
        if(view.begin() != view.end()) {
            return view.front();
        }
        auto mesh_en = reg.create();
        reg.emplace<mesh_cube_builder>(mesh_en, mesh_cube_builder{.size = data::platform_size});
        reg.emplace<mesh_tag>(mesh_en);
        return mesh_en;
    }

    static void create_platform(tree_context &ctx, entity en) {
        auto &reg = ctx.ecs();
        reg.emplace<rendered_mesh>(en, rendered_mesh{.mesh = default_mesh_en(reg), .mat = default_material_en(ctx)});
        reg.get<mut<transform>>(en)->translate(0.5F * data::platform_size.y * vec_down);
    }
};

int main() {
    try {
        app_launcher app{{
            .window = {.size = {800u, 500u}},
            .render = {.culling = false},
        }};
        app.systems()
            .add<ball_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        app.systems()
            .add<camera_system>()
            .run_as<sys_type::start>();
        app.systems()
            .add<platform_system>()
            .run_as<sys_type::start>();
        app.launch();
        return 0;
    } catch(std::exception &e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return -1;
    }
}
#include <algorithm>
#include <cmath>
#include <exception>
#include <iostream>
#include <random>
#include <tuple>
import stay3;
using namespace st;

namespace data {
constexpr vec3f camera_offset{0.F, 2.F, -7.F};
constexpr vec3f platform_size{6.F, 30.F, 30.F};
constexpr auto ball_radius = 1.F;
constexpr auto gravity_scale = 7.F;
constexpr auto left_keys = {scancode::left, scancode::a};
constexpr auto right_keys = {scancode::right, scancode::d};
constexpr auto default_density = 1000.F;
const auto ball_mass = 4 * PI * std::pow(ball_radius, 3.F) / 3.F * default_density;
const auto move_force = 60.F * ball_mass;
const auto stop_force = 3.F * ball_mass;
constexpr auto ball_spawn_ahead_length = 40.F;
constexpr auto platform_spawn_margin = 0.25F;
constexpr auto platform_total_length = static_cast<int>(
    platform_size.z + platform_size.z * 2.F * platform_spawn_margin);
constexpr auto platform_distance_y = 4.F;
constexpr auto platform_rotation_y_max = PI / 4;
constexpr auto platform_rotation_z_max = PI / 8;
} // namespace data

namespace utils {
template<float min, float max>
float random() {
    thread_local std::mt19937 rng{std::random_device{}()};
    thread_local std::uniform_real_distribution<float> dist{min, max};
    return dist(rng);
}
} // namespace utils

struct tags {
    struct ball {};
};

struct camera_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto cam_en = ctx.root().entities().create();
        reg.emplace<main_camera>(cam_en);
        reg.emplace<camera>(cam_en, camera{.far = 100.F, .clear_color = vec4f{32, 41, 56, 255} / 255.F});
        reg.get<mut<transform>>(cam_en)->translate(data::camera_offset / 2.F);
        // DEBUG
        // reg.get<mut<transform>>(cam_en)->translate(vec_right * 30.F).rotate(vec_up, -PI / 2.F);
        // END
    }

    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto tf = reg.get<mut<transform>>(reg.view<camera>().front());
        auto [ball_en, tag, ball_tf] = reg.each<tags::ball, transform>().front();
        const auto desired_position = ball_tf->position() + data::camera_offset;
        tf->set_position(desired_position);
        // DEBUG
        // auto debug_desired_position = ball_tf->position() + (vec_right * 60.F);
        // debug_desired_position.z = 40.F * std::floor(debug_desired_position.z / 40.F);
        // tf->set_position(debug_desired_position).set_orientation(vec_up, -PI / 2);
        // END
    }
};

struct ball_system {
    static void start(tree_context &ctx) {
        create_ball(ctx);
    }

    static entity create_ball(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &texture_cmd = ctx.vars().get<texture_2d::commands>();
        auto ball_en = ctx.root().entities().create();
        reg.emplace<mesh_uv_sphere_builder>(ball_en, mesh_uv_sphere_builder{.radius = data::ball_radius});
        texture_cmd.emplace(texture_2d::command_load{.target = ball_en, .filename = "assets/beach_ball.jpg"});
        reg.emplace<material>(ball_en, material{.texture = ball_en});
        reg.emplace<rendered_mesh>(ball_en, rendered_mesh{.mesh = ball_en, .mat = ball_en});

        reg.emplace<tags::ball>(ball_en);

        reg.get<mut<transform>>(ball_en)->translate(4.F * data::ball_radius * vec_up);
        rigidbody rg{
            .motion_type = rigidbody::type::dynamic,
            .allow_sleep = false,
            .friction = 1.F,
            .linear_damping = 0.3F,
        };
        reg.emplace<rigidbody>(ball_en, rg);
        reg.emplace<collider>(ball_en, sphere{.radius = data::ball_radius});

        return ball_en;
    }

    static void update(seconds delta, tree_context &ctx) {
        auto &window = ctx.vars().get<runtime_info>().window();
        const auto is_key_down = [&window](scancode key) { return window.get_key(key) == key_status::pressed; };
        const auto left = std::ranges::any_of(data::left_keys, is_key_down);
        const auto right = std::ranges::any_of(data::right_keys, is_key_down);

        auto &reg = ctx.ecs();
        auto ball_motion = reg.get<mut<motion>>(reg.view<tags::ball>().front());
        if(left || right) {
            const auto direction = static_cast<float>(left) * (-1.F) + static_cast<float>(right) * 1.F;
            ball_motion->add_force(direction * data::move_force * vec_right);
        } else {
            const auto vx = ball_motion->linear_velocity().x;
            ball_motion->add_force(-vx * data::stop_force * vec_right);
        }
    }
};

struct platform_system {
    void start(tree_context &ctx) {
        auto en = ctx.root().entities().create();
        create_platform(ctx, en, vec3f{}, 0.F, true);
        max_platform_index = 0;
        max_platform_z = data::platform_total_length / 2.F;
        current_platform_x = 0.F;
    }

    int max_platform_index{-1};
    float max_platform_z{};
    float current_platform_x{};

    struct platform_init_args {
        vec3f top_center;
        float rotation;
    };

    void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        const auto [ball_en, tag, ball_tf] = reg.each<tags::ball, transform>().front();
        const auto spawn_ahead_z = static_cast<int>(ball_tf->position().z + data::ball_spawn_ahead_length);
        while(max_platform_z < spawn_ahead_z) {
            auto new_platform_en = ctx.root().entities().create();
            const auto next_platform_tf = calculate_next_transform();
            ++max_platform_index;
            create_platform(ctx, new_platform_en, next_platform_tf.top_center, next_platform_tf.rotation);
        }
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

    static void create_platform(tree_context &ctx, entity en, const vec3f &top_center, float rotation, bool no_rotate = false) {
        auto &reg = ctx.ecs();
        reg.emplace<rendered_mesh>(en, rendered_mesh{.mesh = default_mesh_en(reg), .mat = default_material_en(ctx)});
        {
            auto tf = reg.get<mut<transform>>(en);
            tf->translate(top_center + 0.5F * data::platform_size.y * vec_down);
            if(!no_rotate) {
                tf
                    ->rotate(vec_forward, utils::random<-data::platform_rotation_z_max, data::platform_rotation_z_max>())
                    .rotate(vec_up, rotation);
            }
        }
        constexpr rigidbody rg{
            .motion_type = rigidbody::type::fixed,
            .friction = 1.F,
        };
        reg.emplace<rigidbody>(en, rg);
        reg.emplace<collider>(en, box{data::platform_size});
    }

    platform_init_args calculate_next_transform() {
        const auto angle = utils::random<-data::platform_rotation_y_max, data::platform_rotation_y_max>();
        const auto distance_z = std::cos(angle) * data::platform_total_length;
        const auto distance_x = std::sin(angle) * data::platform_total_length;
        platform_init_args result;
        result.top_center = vec3f{
            current_platform_x + distance_x / 2.F,
            -1.F * (max_platform_index + 1) * data::platform_distance_y,
            max_platform_z + distance_z / 2.F,
        };
        result.rotation = angle;
        current_platform_x += distance_x;
        max_platform_z += distance_z;
        return result;
    }
};

int main() {
    try {
        app_launcher app{{
            .window = {.size = {800u, 500u}, .name = "Slope clone"},
            .render = {.culling = false},
            .physics = {.gravity = (vec_down + vec_forward) * data::gravity_scale},
        }};
        app.systems()
            .add<ball_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        app.systems()
            .add<camera_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        app.systems()
            .add<platform_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        app.launch();
        return 0;
    } catch(std::exception &e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return -1;
    }
}
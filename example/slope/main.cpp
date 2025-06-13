#include <algorithm>
#include <cassert>
#include <cmath>
#include <exception>
#include <iostream>
#include <map>
#include <random>
#include <ranges>
#include <string>
#include <tuple>
#include <unordered_set>
import stay3;
using namespace st;

namespace data {
constexpr vec3f camera_offset{0.F, 2.F, -7.F};
constexpr vec3f platform_size{6.F, 2.F, 50.F};
constexpr auto ball_radius = 1.F;
constexpr vec3f gravity{0.F, -18.F, 5.F};
constexpr auto left_keys = {scancode::left, scancode::a};
constexpr auto right_keys = {scancode::right, scancode::d};
constexpr auto default_density = 1000.F;
const auto ball_mass = 4 * PI * std::pow(ball_radius, 3.F) / 3.F * default_density;
const auto move_force = 60.F * ball_mass;
const auto stop_force = 0.2F * ball_mass;
const vec3f boost_impulse = vec3f{0.F, 15.F, 6.F} * ball_mass;
const auto boost_stop_force = 0.1F * ball_mass;
constexpr auto ball_spawn_ahead_length = 100.F;
constexpr auto platform_spawn_margin = 0.2F;
constexpr auto platform_total_length = static_cast<int>(
    platform_size.z + platform_size.z * 2.F * platform_spawn_margin);
constexpr auto platform_distance_y = 6.5F;
constexpr auto platform_rotation_y_max = PI / 4;
constexpr auto platform_rotation_z_max = PI / 10;
constexpr auto platform_behind_camera_retain_length = 2 * platform_total_length;
} // namespace data

namespace utils {
template<float min, float max>
float random() {
    thread_local std::mt19937 rng{std::random_device{}()};
    thread_local std::uniform_real_distribution<float> dist{min, max};
    return dist(rng);
}
std::u8string int_to_str(int value) {
    std::string str = std::to_string(value);
    return {str.begin(), str.end()};
}
} // namespace utils

struct tags {
    struct ball {};
    struct jump_sensor {};
    struct game_over_sensor {};
    struct font_asset {};
};

struct game {
    enum class state : std::uint8_t {
        boot,
        play,
        play_to_over,
        over,
    };
    [[nodiscard]] static bool is_state(state st, ecs_registry &reg) {
        return current_state(reg) == st;
    }
    [[nodiscard]] static state current_state(ecs_registry &reg) {
        return *std::get<1>(reg.each<state>().front());
    }
    static void to_state(state st, ecs_registry &reg) {
        assert(st != current_state(reg));
        auto [en, cur] = reg.each<mut<state>>().front();
        *cur = st;
    }
};

struct camera_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto cam_en = ctx.root().add_child().entities().create();
        reg.emplace<main_camera>(cam_en);
        reg.emplace<camera>(cam_en, camera{.far = 100.F, .clear_color = vec4f{32, 41, 56, 255} / 255.F});
    }

    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(game::is_state(game::state::play, reg)) {
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
    }
};

struct ball_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::update, game::state>().connect<+[](tree_context &ctx, ecs_registry &reg, entity) {
            if(game::is_state(game::state::play, reg)) {
                create_ball(ctx);
            }
            if(game::is_state(game::state::over, reg)) {
                destroy_ball(ctx);
            }
        }>(ctx);
    }

    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(game::is_state(game::state::play, reg)) {
            auto [en, ball_motion, tag] = reg.each<mut<motion>, tags::ball>().front();
            update_horizontal_movement(ctx, *ball_motion);
            update_vertical_movement(*ball_motion);
        }
    }

private:
    struct resource_tag {};
    static entity ball_resources_entity(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto view = reg.view<resource_tag>();
        if(view.begin() != view.end()) { return view.front(); }

        auto result = reg.create();
        auto &texture_cmd = ctx.vars().get<texture_2d::commands>();
        reg.emplace<mesh_uv_sphere_builder>(result, mesh_uv_sphere_builder{.radius = data::ball_radius});
        texture_cmd.emplace(texture_2d::command_load{.target = result, .filename = "assets/beach_ball.jpg"});
        reg.emplace<material>(result, material{.texture = result});
        reg.emplace<resource_tag>(result);
        return result;
    }

    static entity create_ball(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto ball_en = ctx.root().entities().create();
        auto resource_en = ball_resources_entity(ctx);
        reg.emplace<rendered_mesh>(ball_en, rendered_mesh{.mesh = resource_en, .mat = resource_en});
        reg.emplace<tags::ball>(ball_en);
        reg.get<mut<transform>>(ball_en)->translate(4.F * data::ball_radius * vec_up);
        rigidbody rg{
            .motion_type = rigidbody::type::dynamic,
            .allow_sleep = false,
            .friction = 1.F,
            .linear_damping = 0.2F,
            .angular_damping = 0.5F,
        };
        reg.emplace<rigidbody>(ball_en, rg);
        reg.emplace<collider>(ball_en, sphere{.radius = data::ball_radius});
        reg.emplace<collision_enter>(ball_en);

        return ball_en;
    }

    static void destroy_ball(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto view = reg.view<tags::ball>();
        reg.destroy(view.front());
    }

    static void update_vertical_movement(motion &ball_motion) {
        auto velocity = ball_motion.linear_velocity().z;
        if(velocity > 0.F) {
            ball_motion.add_force(vec_back * velocity * data::boost_stop_force);
        }
    }

    static void update_horizontal_movement(tree_context &ctx, motion &ball_motion) {
        auto &window = ctx.vars().get<runtime_info>().window();
        const auto is_key_down = [&window](scancode key) { return window.get_key(key) == key_status::pressed; };
        const auto left = std::ranges::any_of(data::left_keys, is_key_down);
        const auto right = std::ranges::any_of(data::right_keys, is_key_down);

        auto &reg = ctx.ecs();
        if(left || right) {
            const auto direction = static_cast<float>(left) * (-1.F) + static_cast<float>(right) * 1.F;
            ball_motion.add_force(direction * data::move_force * vec_right);
        } else {
            const auto vx = ball_motion.linear_velocity().x;
            ball_motion.add_force(-vx * data::stop_force * vec_right);
        }
    }
};

class platform_system {
    struct game_start_payload {
        platform_system *sys;
        tree_context *ctx;
    } game_start_pl;
    int max_platform_index{};
    std::map<int, node *> index_to_platform_node{};
    float max_platform_z{};
    float current_platform_x{};
    node *platform_node{};

public:
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        platform_node = &ctx.root().add_child();
        game_start_pl = {.sys = this, .ctx = &ctx};
        reg.on<comp_event::update, game::state>().connect<+[](game_start_payload &pl, ecs_registry &reg, entity) {
            if(game::is_state(game::state::play, reg)) {
                pl.sys->on_round_start(*pl.ctx);
            }
        }>(game_start_pl);
    }

    void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(game::is_state(game::state::play, reg)) {
            const auto [ball_en, tag, ball_tf] = reg.each<tags::ball, transform>().front();
            spawn_platforms(ctx, ball_tf->position());
            check_sensor_boost(ball_en, reg);
            remove_back_platforms(reg);
        }
    }

private:
    void on_round_start(tree_context &ctx) {
        destroy_previous_platforms();
        auto &initial_platform = create_platform(ctx, vec3f{}, 0.F, true);
        max_platform_index = 0;
        index_to_platform_node.clear();
        index_to_platform_node.emplace(0, &initial_platform);
        max_platform_z = data::platform_total_length / 2.F;
        current_platform_x = 0.F;
    }
    static void check_sensor_boost(entity ball, ecs_registry &reg) {
        auto ball_motion = reg.get<mut<motion>>(ball);
        for(const auto &col: *reg.get<collision_enter>(ball)) {
            const auto is_sensor = reg.contains<tags::jump_sensor>(col.other);
            if(is_sensor) {
                ball_motion->add_impulse(data::boost_impulse);
            }
        }
    }

    void spawn_platforms(tree_context &ctx, const vec3f &ball_position) {
        const auto spawn_ahead_z = static_cast<int>(ball_position.z + data::ball_spawn_ahead_length);
        while(max_platform_z < spawn_ahead_z) {
            const auto next_platform_tf = calculate_next_transform();
            ++max_platform_index;
            auto &node = create_platform(ctx, next_platform_tf.top_center, next_platform_tf.rotation);
            index_to_platform_node.emplace(max_platform_index, &node);
        }
    }

    void destroy_previous_platforms() {
        platform_node->destroy_children();
    }

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

    node &create_platform(tree_context &ctx, const vec3f &top_center, float rotation, bool no_rotate = false) {
        auto &reg = ctx.ecs();
        auto &node = platform_node->add_child();
        auto en = node.entities().create();
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
            .allow_sleep = false,
            .friction = 1.F,
        };
        reg.emplace<rigidbody>(en, rg);
        reg.emplace<collider>(en, box{data::platform_size});

        auto &sensor_node = node.add_child();

        auto jump_sensor_en = sensor_node.entities().create();
        reg.emplace<rigidbody>(jump_sensor_en, rigidbody{.motion_type = rigidbody::type::fixed, .is_sensor = true, .allow_sleep = false});
        reg.emplace<collider>(jump_sensor_en, box{data::platform_size.x * 1.5F, 4 * data::ball_radius, 4 * data::ball_radius});
        reg.get<mut<transform>>(jump_sensor_en)
            ->translate(vec_up * data::platform_size.y / 2.F)
            .translate(vec_forward * (data::platform_size.z / 2.F - 2 * data::ball_radius));
        reg.emplace<tags::jump_sensor>(jump_sensor_en);

        auto game_over_sensor_en = sensor_node.entities().create();
        reg.emplace<rigidbody>(game_over_sensor_en, rigidbody{.motion_type = rigidbody::type::fixed, .is_sensor = true, .allow_sleep = false});
        reg.emplace<collider>(game_over_sensor_en, box{data::platform_size.x * 100, data::platform_size.y / 2, data::platform_total_length});
        reg.get<mut<transform>>(game_over_sensor_en);
        reg.emplace<tags::game_over_sensor>(game_over_sensor_en);

        return node;
    }

    void remove_back_platforms(ecs_registry &reg) {
        const auto &camera_position = reg.get<transform>(reg.view<camera>().front())->position();
        while(!index_to_platform_node.empty()) {
            auto it = index_to_platform_node.begin();
            auto first_entity = *it->second->entities().begin();
            const auto &position = reg.get<transform>(first_entity)->position();
            if(position.z < camera_position.z - data::platform_behind_camera_retain_length) {
                platform_node->destroy_child(it->second->id());
                index_to_platform_node.erase(it);
            } else {
                break;
            }
        }
    }

    struct platform_init_args {
        vec3f top_center;
        float rotation;
    };
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

class score_system {
    struct game_start_payload {
        score_system *sys{};
        tree_context *ctx{};
    } game_start_pl;
    int score{};
    std::unordered_set<entity, entity_hasher, entity_equal> collided_platforms;

    struct score_tag {};

public:
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        game_start_pl = {.sys = this, .ctx = &ctx};
        reg.on<comp_event::update, game::state>().connect<+[](game_start_payload &pl, ecs_registry &reg, entity) {
            if(game::is_state(game::state::boot, reg)) {
                auto cam_en = reg.view<camera>().front();
                auto &score_text_node = pl.ctx->get_node(cam_en).add_child();
                auto score_text_en = score_text_node.entities().create();
                reg.emplace<score_tag>(score_text_en);
                reg.emplace<mut<transform>>(score_text_en, vec_forward + vec_up * 0.4F)->scale(0.15F);
            } else if(game::is_state(game::state::play, reg)) {
                pl.sys->score = 0;
                pl.sys->collided_platforms.clear();
                reg.emplace_or_replace<text>(
                    reg.view<score_tag>().front(),
                    text{
                        .content = u8"0",
                        .size = 60,
                        .font = reg.view<tags::font_asset>().front(),
                    });
            }
        }>(game_start_pl);
    }

    void update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(game::is_state(game::state::play, reg)) {
            auto [ball_en, collision_list, tag] = reg.each<collision_enter, tags::ball>().front();
            for(const auto &col: *collision_list) {
                if(col.normal.y > -0.1F) {
                    continue;
                }
                if(reg.get<rigidbody>(col.other)->is_sensor) {
                    continue;
                }
                if(collided_platforms.contains(col.other)) {
                    continue;
                }
                collided_platforms.insert(col.other);
                ++score;
                reg.get<mut<text>>(reg.view<score_tag>().front())->content = utils::int_to_str(score);
            }
        }
    }
};

struct game_over_system {
    void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(game::is_state(game::state::play, reg)) {
            if(is_game_over(ctx)) {
                game::to_state(game::state::play_to_over, reg);
                remaining_duration = game_over_duration;
            }
        } else if(game::is_state(game::state::play_to_over, reg)) {
            remaining_duration -= delta;
            if(remaining_duration <= 0.F) {
                game::to_state(game::state::over, reg);
            }
        }
    }

private:
    static bool is_game_over(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto [ball_en, collision_list, tag] = reg.each<collision_enter, tags::ball>().front();
        return std::ranges::any_of(*collision_list, [&reg](const collision &col) {
            return reg.contains<tags::game_over_sensor>(col.other);
        });
    }

    static constexpr auto game_over_duration = seconds{2.f};
    float remaining_duration{};
};

class info_text_system {
    struct space_to_play {};
    struct space_to_replay {};
    static constexpr auto text_size = 40;

public:
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto font_en = ctx.root().entities().create();
        reg.emplace<font>(font_en, "assets/stay3/fonts/Roboto-Regular.ttf");
        reg.emplace<tags::font_asset>(font_en);

        reg.on<comp_event::update, game::state>().connect<+[](tree_context &ctx, ecs_registry &reg, entity) {
            auto &cam_node = ctx.get_node(reg.view<camera>().front());
            if(game::is_state(game::state::boot, reg)) {
                auto text_en = cam_node.add_child().entities().create();
                reg.emplace<mut<transform>>(text_en, vec_forward)->scale(0.15F);
                reg.emplace<text>(
                    text_en,
                    text{
                        .content = u8"Press SPACE to start",
                        .size = text_size,
                        .font = reg.view<tags::font_asset>().front(),
                    });
                reg.emplace<space_to_play>(text_en);
            } else if(game::is_state(game::state::play, reg)) {
                auto view1 = reg.view<space_to_play>();
                auto view2 = reg.view<space_to_replay>();
                if(view1.begin() != view1.end()) {
                    cam_node.destroy_child(ctx.get_node(view1.front()).id());
                }
                if(view2.begin() != view2.end()) {
                    cam_node.destroy_child(ctx.get_node(view2.front()).id());
                }
            } else if(game::is_state(game::state::over, reg)) {
                auto text_en = cam_node.add_child().entities().create();
                reg.emplace<mut<transform>>(text_en, vec_forward)->scale(0.15F);
                reg.emplace<text>(
                    text_en,
                    text{
                        .content = u8"You lose. Press SPACE to restart",
                        .size = text_size,
                        .font = reg.view<tags::font_asset>().front(),
                    });
                reg.emplace<space_to_replay>(text_en);
            }
        }>(ctx);
    }
};

struct game_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto game_state_en = reg.create();
        reg.emplace<mut<game::state>>(game_state_en, game::state::boot);
    }

    static void input(const event &ev, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(game::is_state(game::state::boot, reg)) {
            if(const auto *key = ev.try_get<event::key_pressed>(); key != nullptr) {
                if(key->code == scancode::space) {
                    game::to_state(game::state::play, reg);
                }
            }
        } else if(game::is_state(game::state::over, reg)) {
            if(const auto *key = ev.try_get<event::key_pressed>(); key != nullptr) {
                if(key->code == scancode::space) {
                    game::to_state(game::state::play, reg);
                }
            }
        }
    }
};

int main() {
    try {
        app_launcher app{{
            .window = {.size = {800u, 500u}, .name = "Slope clone"},
            .render = {.culling = true},
            .physics = {
                .gravity = data::gravity,
                .debug_draw = false,
            },
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
        app.systems()
            .add<score_system>()
            .run_as<sys_type::start>(sys_priority::low)
            .run_as<sys_type::update>();
        app.systems()
            .add<game_over_system>()
            .run_as<sys_type::update>();
        app.systems()
            .add<info_text_system>()
            .run_as<sys_type::start>();
        app.systems()
            .add<game_system>()
            .run_as<sys_type::start>(sys_priority::very_low)
            .run_as<sys_type::input>();
        app.launch();
        return 0;
    } catch(std::exception &e) {
        std::cerr << "Exception: " << e.what() << '\n';
        return -1;
    }
}
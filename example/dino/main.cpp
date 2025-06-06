/**
 * @brief The clone of the dino game of Google
 * @note It is currently a w.i.p
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <random>
#include <unordered_map>
#include <vector>

import stay3;

using namespace st;

namespace data {
enum class sprite : std::uint8_t {
    dino_wait,
    dino_run_1,
    dino_run_2,
    dino_run_duck_1,
    dino_run_duck_2,
    dino_jump,
    ground,
};
const std::unordered_map<sprite, rectf> rects = {
    {sprite::dino_wait, {.position = {76.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_1, {.position = {1854.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_2, {.position = {1942.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_duck_1, {.position = {2324.F, 2.F}, .size = {118.F, 94.F}}},
    {sprite::dino_run_duck_2, {.position = {2206.F, 2.F}, .size = {118.F, 94.F}}},
    {sprite::dino_jump, {.position = {1678.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::ground, {.position = {2.F, 104.F}, .size = {2400.F, 24.F}}},
};
constexpr auto pixels_per_unit = 100.F;
constexpr vec3f gravity{0.F, -30.F, 0.F};
constexpr vec3f dino_start_offset = 5.F * vec_left;
constexpr auto dino_jump_height = 1.5F;
const vec2f dino_bounding_box = vec2f{40.F, 94.F} / pixels_per_unit;
const std::vector dino_run_frames = {
    rects.at(sprite::dino_run_1),
    rects.at(sprite::dino_run_2),
};
const std::vector dino_run_duck_frames = {
    rects.at(sprite::dino_run_duck_1),
    rects.at(sprite::dino_run_duck_2),
};
enum class animations : std::uint8_t {
    dino_run,
    dino_run_duck,
    dino_jump,
};
constexpr auto dino_fps = 8.F;
constexpr auto jump_buttons = {scancode::enter, scancode::w, scancode::up, scancode::space};
constexpr auto duck_buttons = {scancode::down, scancode::s};
} // namespace data

struct my_rigidbody {
    vec3f velocity;
    vec3f acceleration{data::gravity};
};
struct touched_ground {};
struct jump_data {
    float height{};
};
struct jumped {};
struct bounding_box {
    vec2f size;
};
struct bounding_box_debug {
    entity renderer;
};
struct animation_data {
    std::vector<rectf> frames;
    float fps{};
};
struct animation_user {
    component_ref<animation_data> animation;
};
struct animation_user_state {
    decltype(animation_data::frames)::size_type index{};
    float current_time{};
};
struct dino_first_touched_ground {};
struct ground_reveal_started {};
struct ground_revealing {
    float speed{};
    float max_width{};
    float current_width{};
};
struct ground_revealed {};
using animation_holders = std::unordered_map<data::animations, entity>;
struct texture_holder {
    entity holder;
};
struct material_holder {
    entity holder;
};
enum class dino_state : std::uint8_t {
    idle,
    run,
    run_duck,
    jump,
};
const std::unordered_map<data::animations, animation_data> animations{
    {data::animations::dino_run, {.frames = data::dino_run_frames, .fps = data::dino_fps}},
    {data::animations::dino_run_duck, {.frames = data::dino_run_duck_frames, .fps = data::dino_fps}},
    {data::animations::dino_jump, {.frames = {data::rects.at(data::sprite::dino_jump)}, .fps = data::dino_fps}},
};

animation_holders create_animations(ecs_registry &reg, node &resource_node) {
    animation_holders result;
    for(auto &&[animation_key, data]: animations) {
        const auto en = resource_node.entities().create();
        reg.emplace<animation_data>(en, data);
        result.emplace(animation_key, en);
    }
    return result;
}

bool is_jump_button(scancode code) {
    return std::ranges::contains(data::jump_buttons, code);
}
bool is_jump_button_pressed(glfw_window &window) {
    return std::ranges::any_of(data::jump_buttons, [&window](scancode code) {
        return window.get_key(code) == key_status::pressed;
    });
}
bool is_duck_button(scancode code) {
    return std::ranges::contains(data::duck_buttons, code);
}
bool is_duck_button_pressed(glfw_window &window) {
    return std::ranges::any_of(data::duck_buttons, [&window](scancode code) {
        return window.get_key(code) == key_status::pressed;
    });
}

class simple_physics_system {
public:
    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, tf, rg]: reg.each<mut<transform>, mut<my_rigidbody>>()) {
            rg->velocity += rg->acceleration * delta;
            tf->translate(rg->velocity * delta);
        }
        for(auto [en, tf, rg, box]: reg.each<mut<transform>, my_rigidbody, bounding_box>()) {
            const auto bottom = tf->position().y - (box->size.y / 2.F);
            if(bottom < 0) {
                tf->translate(vec_up * (-bottom));
                reg.get<mut<my_rigidbody>>(en)->velocity.y = std::max(0.F, rg->velocity.y);
                reg.emplace_if_not_exist<touched_ground>(en);
            } else {
                reg.destroy_if_exist<touched_ground>(en);
            }
        }
    }
};

class animation_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, animation_user>().connect<&animation_system::on_animation_user_constructed>();
        reg.on<comp_event::update, animation_user>().connect<&animation_system::update_animation_mesh>();
        reg.on<comp_event::destroy, animation_user>().connect<&ecs_registry::destroy_if_exist<animation_user_state>>();
    }
    static void post_update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, state, data]: reg.each<mut<animation_user_state>, animation_user>()) {
            state->current_time += delta;
            auto animation = data->animation.get(reg);
            const auto is_frame_modified = state->current_time > 1.F / animation->fps;
            while(state->current_time > 1.F / animation->fps) {
                state->current_time -= 1.F / animation->fps;
                state->index = (state->index + 1) % animation->frames.size();
            }
            if(is_frame_modified) {
                update_animation_mesh(reg, en);
            }
        }
    }

private:
    static void on_animation_user_constructed(ecs_registry &reg, entity en) {
        auto anim = reg.get<animation_user>(en);
        assert(reg.contains<mesh_sprite_builder>(en) && "Expect animation_user to have mesh_sprite_builder");
        reg.emplace<animation_user_state>(en);
        update_animation_mesh(reg, en);
        reg.get<mut<rendered_mesh>>(en)->mesh = en;
    }
    static void update_animation_mesh(ecs_registry &reg, entity en) {
        auto [mesh_builder, state, data] = reg.get<mut<mesh_sprite_builder>, mut<animation_user_state>, animation_user>(en);
        auto animation = data->animation.get(reg);
        assert(!animation->frames.empty() && "Empty animation data");
        state->index %= animation->frames.size();
        mesh_builder->texture_rect = animation->frames[state->index];
    }
};

class bound_debug_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, bounding_box>().connect<&bound_debug_system::on_bounding_box_constructed>(ctx);
        reg.on<comp_event::destroy, bounding_box_debug>().connect<&bound_debug_system::remove_shape_entity>();
    }
    static void post_update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, db, tf]: reg.each<bounding_box_debug, transform>()) {
            auto renderer_tf = reg.get<mut<transform>>(db->renderer);
            *renderer_tf = *tf;
        }
    }

private:
    static void on_bounding_box_constructed(tree_context &ctx, ecs_registry &reg, entity en) {
        const auto box = reg.get<bounding_box>(en);
        auto db = reg.emplace<mut<bounding_box_debug>>(en);
        db->renderer = add_shape_entity(ctx, ctx.get_node(en), box->size, get_random_color());
    }
    static entity add_shape_entity(tree_context &ctx, node &bounding_box_node, const vec2f &size, const vec4f &color) {
        auto &reg = ctx.ecs();
        auto en = bounding_box_node.entities().create();
        const auto &material = ctx.vars().get<material_holder>();
        reg.emplace<mesh_plane_builder>(en, mesh_plane_builder{.size = size, .color = color});
        reg.emplace<rendered_mesh>(en, rendered_mesh{.mesh = en, .mat = material.holder});
        return en;
    }
    static void remove_shape_entity(ecs_registry &reg, entity en) {
        const auto db_entity = reg.get<bounding_box_debug>(en)->renderer;
        reg.destroy_if_exist(db_entity);
    }
    static vec4f get_random_color() {
        thread_local std::random_device rd;
        thread_local std::mt19937 gen{rd()};
        thread_local std::uniform_real_distribution<float> dist{0.F, 1.F};

        return {dist(gen), dist(gen), dist(gen), 1.F};
    }
};

class jump_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, touched_ground>().connect<&ecs_registry::destroy_if_exist<jumped>>();
    }
    static void update(seconds, tree_context &ctx) {
        auto &window = ctx.vars().get<runtime_info>().window();
        if(is_jump_button_pressed(window)) {
            jump_all(ctx.ecs());
        }
    }

private:
    static void jump_all(ecs_registry &reg) {
        for(auto [en, rg, jump]: reg.each<mut<my_rigidbody>, jump_data>()) {
            if(reg.contains<touched_ground>(en)) {
                rg->velocity.y = std::sqrt(std::abs(2.F * jump->height * data::gravity.y));
                reg.emplace_if_not_exist<jumped>(en);
            }
        }
    }
};

class dino_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, jumped>().connect<&dino_system::to_jump_state>(ctx);
        reg.on<comp_event::construct, touched_ground>().connect<&dino_system::on_touched_ground>(ctx);
    }
    static void update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &window = ctx.vars().get<runtime_info>().window();
        const auto duck_button_pressed = is_duck_button_pressed(window);

        for(auto [en, dino]: reg.each<mut<dino_state>>()) {
            switch(*dino) {
            case dino_state::run: {
                if(duck_button_pressed) {
                    to_duck_state(ctx, reg, en);
                }
                break;
            }
            case dino_state::run_duck:
                if(!duck_button_pressed) {
                    to_run_state(ctx, reg, en);
                }
                break;
            default:
                break;
            }
        }
    }

private:
    static void to_jump_state(tree_context &ctx, ecs_registry &reg, entity en) {
        if(!reg.contains<dino_state>(en)) { return; }
        *reg.get<mut<dino_state>>(en) = dino_state::jump;
        reg.get<mut<animation_user>>(en)->animation = ctx.vars().get<animation_holders>().at(data::animations::dino_jump);
    }
    static void to_run_state(tree_context &ctx, ecs_registry &reg, entity en) {
        if(!reg.contains<dino_state>(en)) { return; }
        *reg.get<mut<dino_state>>(en) = dino_state::run;
        reg.get<mut<animation_user>>(en)->animation = ctx.vars().get<animation_holders>().at(data::animations::dino_run);
    }
    static void to_duck_state(tree_context &ctx, ecs_registry &reg, entity en) {
        if(!reg.contains<dino_state>(en)) { return; }
        *reg.get<mut<dino_state>>(en) = dino_state::run_duck;
        reg.get<mut<animation_user>>(en)->animation = ctx.vars().get<animation_holders>().at(data::animations::dino_run_duck);
    }
    static void on_touched_ground(tree_context &ctx, ecs_registry &reg, entity en) {
        if(!reg.contains<dino_state>(en)) { return; }
        if(*reg.get<dino_state>(en) == dino_state::jump) {
            to_run_state(ctx, reg, en);
            reg.emplace_if_not_exist<dino_first_touched_ground>(en);
        }
    }
};

class ground_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, ground_reveal_started>().connect<&ground_system::prepare_ground_render>(ctx);
        reg.on<comp_event::construct, ground_reveal_started>().connect<+[](ecs_registry &reg, entity en) {
            reg.emplace<ground_revealing>(
                en,
                ground_revealing{
                    .speed = 20.F,
                    .max_width = 2.F * std::abs(data::dino_start_offset.x),
                });
        }>();
    }
    static void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto &&[en, revealing, mesh_builder]: reg.each<mut<ground_revealing>, mut<mesh_sprite_builder>>()) {
            const auto new_width_unclamped = revealing->current_width + (revealing->speed * delta);
            const auto is_max_width_reached = new_width_unclamped >= revealing->max_width;
            revealing->current_width = std::min(revealing->max_width, new_width_unclamped);
            mesh_builder->texture_rect->size.x = revealing->current_width * mesh_builder->pixels_per_unit;
            if(is_max_width_reached) {
                reg.emplace_if_not_exist<ground_revealed>(en);
            }
        }
        for(auto &&[en, revealed, mesh_builder]: reg.each<ground_revealed, mut<mesh_sprite_builder>>()) {
            const auto speed = 10.F;
            auto &texture_position = mesh_builder->texture_rect->position;
            texture_position.x += speed * delta * mesh_builder->pixels_per_unit;
            const auto &texture_rect = data::rects.at(data::sprite::ground);
            texture_position.x = std::fmod(texture_position.x, texture_rect.size.x) + texture_rect.position.x;
        }
    }

private:
    static void prepare_ground_render(tree_context &ctx, ecs_registry &reg, entity en) {
        const auto &vars = ctx.vars();
        const auto &material = vars.get<material_holder>();
        const auto &texture = vars.get<texture_holder>();
        {
            auto builder = reg.emplace<mut<mesh_sprite_builder>>(
                en,
                mesh_sprite_builder{
                    .texture = texture.holder,
                    .pixels_per_unit = data::pixels_per_unit,
                    .origin = {},
                    .texture_rect = data::rects.at(data::sprite::ground),
                });
            builder->texture_rect->size.x = 0.F;
        }
        reg.emplace<rendered_mesh>(en, rendered_mesh{.mesh = en, .mat = material.holder});
        reg.get<mut<transform>>(en)
            ->translate(data::dino_start_offset)
            .translate(vec_forward * 2.F)
            .translate(vec_up * 0.3F);
    }
};

class game_system {
public:
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &vars = ctx.vars();
        auto &texture_cmd = vars.get<texture_2d::commands>();

        auto &resource_node = ctx.root().add_child();
        auto &scene_node = ctx.root().add_child();

        auto texture_en = resource_node.entities().create();
        texture_cmd.emplace(texture_2d::command_load{
            .target = texture_en,
            .filename = "assets/base64.png",
        });
        vars.emplace<texture_holder>(texture_en);
        auto material_en = resource_node.entities().create();
        reg.emplace<material>(material_en, material{.texture = texture_en});
        vars.emplace<material_holder>(material_en);
        vars.emplace<animation_holders>(create_animations(reg, resource_node));

        auto cam = scene_node.entities().create();
        reg.emplace<camera>(
            cam,
            camera{
                .clear_color = vec4f{1.F},
                .data = camera::orthographic_data{
                    .width = 1.2F * 2.F * std::abs(data::dino_start_offset.x),
                },
            });
        reg.emplace<main_camera>(cam);
        reg.get<mut<transform>>(cam)->set_position(vec_back * 3.F + 1.5F * vec_up);

        m_dino = scene_node.entities().create();
        reg.emplace<mesh_sprite_builder>(
            m_dino,
            mesh_sprite_builder{
                .texture = texture_en,
                .pixels_per_unit = data::pixels_per_unit,
                .origin = vec2f{0.F, 0.5F},
                .texture_rect = data::rects.at(data::sprite::dino_wait),
            });
        reg.emplace<rendered_mesh>(
            m_dino,
            rendered_mesh{
                .mesh = m_dino,
                .mat = material_en,
            });
        reg.get<mut<transform>>(m_dino)
            ->translate(data::rects.at(data::sprite::dino_wait).size.y * 0.5F / data::pixels_per_unit * vec_up)
            .translate(data::dino_start_offset);
        reg.emplace<dino_state>(m_dino, dino_state::idle);
        reg.emplace<bounding_box>(m_dino, data::dino_bounding_box);

        m_ground = scene_node.entities().create();
    }

    void input(const event &ev, tree_context &ctx) {
        switch(m_current_state) {
        case state::idle:
            if(const auto *key = ev.try_get<event::key_pressed>(); key != nullptr && is_jump_button(key->code)) {
                to_transition(ctx, ctx.ecs());
            }
            break;
        default:
            break;
        }
    }

private:
    enum class state : std::uint8_t {
        idle,
        transition,
        game,
        over,
    };

    void to_transition(tree_context &ctx, ecs_registry &reg) {
        m_current_state = state::transition;
        reg.emplace<my_rigidbody>(m_dino);
        reg.emplace<jump_data>(m_dino, jump_data{.height = data::dino_jump_height});
        const auto &animations = ctx.vars().get<animation_holders>();
        reg.emplace<animation_user>(m_dino, animation_user{.animation = animations.at(data::animations::dino_jump)});

        reg.on<comp_event::construct, dino_first_touched_ground>().connect<+[](entity ground, ecs_registry &reg, entity) {
            reg.emplace<ground_reveal_started>(ground);
        }>(m_ground);
    }

    state m_current_state{state::idle};
    entity m_dino;
    entity m_ground;
};

int main() {
    constexpr vec2u win_size{800u, 300u};
    constexpr auto updates_per_sec = 120;
    try {
        app_launcher my_app{{
            .window = {
                .size = win_size,
                .name = "Dino game",
            },
            .updates_per_second = updates_per_sec,
            .render = {
                .power_pref = render_config::power_preference::low,
                .filter = filter_mode::nearest,
            },
        }};
        auto &systems = my_app.systems();
        systems
            .add<game_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::input>();
        systems
            .add<simple_physics_system>()
            .run_as<sys_type::update>(sys_priority::high);
        systems
            .add<jump_system>()
            .run_as<sys_type::update>()
            .run_as<sys_type::start>();
        systems
            .add<bound_debug_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::post_update>();
        systems
            .add<animation_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::post_update>();
        systems
            .add<dino_system>()
            .run_as<sys_type::update>()
            .run_as<sys_type::start>();
        systems
            .add<ground_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        my_app.launch();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

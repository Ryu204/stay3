#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
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
};
const std::unordered_map<sprite, rectf> rects = {
    {sprite::dino_wait, {.position = {76.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_1, {.position = {1854.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_2, {.position = {1942.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_duck_1, {.position = {2324.F, 2.F}, .size = {118.F, 94.F}}},
    {sprite::dino_run_duck_2, {.position = {2206.F, 2.F}, .size = {118.F, 94.F}}},
    {sprite::dino_jump, {.position = {1678.F, 2.F}, .size = {88.F, 94.F}}},
};
constexpr auto pixels_per_unit = 100.F;
constexpr vec3f gravity{0.F, -30.F, 0.F};
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
constexpr auto dino_fps = 8.F;
constexpr auto jump_buttons = {scancode::enter, scancode::w, scancode::up, scancode::space};
constexpr auto duck_buttons = {scancode::down, scancode::s};
} // namespace data

void create_mesh_data(data::sprite id, const texture_2d_data &texture, entity en, ecs_registry &reg) {
    reg.add_component<const mesh_data>(en, mesh_sprite(texture, data::pixels_per_unit, vec4f{1.F}, data::rects.at(id)));
}

std::unordered_map<data::sprite, entity> create_meshes(ecs_registry &reg, node &resource_node, const texture_2d_data &texture) {
    std::unordered_map<data::sprite, entity> result;
    std::array sprites = {data::sprite::dino_wait};
    for(auto spr: sprites) {
        const auto en = resource_node.entities().create();
        create_mesh_data(spr, texture, en, reg);
        result.emplace(spr, en);
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

struct velocity {
    vec3f value;
};
struct jumpable {
    float height{};
};
struct can_jump {};
struct jumped {};
struct bounding_box {
    vec2f size;
};

struct animation {
    std::vector<rectf> frames;
    float fps{};
};
struct animation_status {
    decltype(animation::frames)::size_type index{};
    float current_time{};
};

enum class dino_state : std::uint8_t {
    idle,
    run,
    run_duck,
    jump,
};

class movement_system {
public:
    static void update(seconds delta, tree_context &ctx) {
        for(auto [en, vel]: ctx.ecs().each<velocity>()) {
            vel->value += data::gravity * delta;
        }
        for(auto [en, tf, vel]: ctx.ecs().each<transform, const velocity>()) {
            tf->translate(vel->value * delta);
        }
    }
};

class animation_system {
    static void update_anim_frame(ecs_registry &reg, const animation &anim, animation_status &anim_status, const rendered_mesh &rmesh) {
        const auto texture_holder = reg.get_components<const material_data>(rmesh.material_holder)->texture_holder;
        const auto &texture = *reg.get_components<const texture_2d_data>(texture_holder);
        auto mesh = reg.get_components<mesh_data>(rmesh.mesh_holder);
        anim_status.index %= anim.frames.size();
        *mesh = mesh_sprite(texture, data::pixels_per_unit, vec4f{1.F}, anim.frames[anim_status.index]);
    }

public:
    static void start(tree_context &ctx) {
        ctx.ecs().on<comp_event::update, animation>().connect<+[](ecs_registry &reg, entity en) {
            update_anim_frame(
                reg,
                *reg.get_components<const animation>(en),
                *reg.get_components<animation_status>(en),
                *reg.get_components<const rendered_mesh>(en));
        }>();
        make_hard_dependency<animation_status, animation>(ctx.ecs());
    }
    static void post_update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, anim, anim_status, rmesh]: reg.each<const animation, animation_status, const rendered_mesh>()) {
            anim_status->current_time += delta;
            bool changed{false};
            while(anim_status->current_time > 1.F / anim->fps) {
                anim_status->current_time -= 1.F / anim->fps;
                ++anim_status->index;
                anim_status->index %= anim->frames.size();
                changed = true;
            }
            if(!changed) { return; }
            update_anim_frame(reg, *anim, *anim_status, *rmesh);
        }
    }
};

class bound_system {
public:
    static void post_update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, tf, box]: reg.each<transform, const bounding_box>()) {
            const auto bottom = tf->position().y - (box->size.y / 2.F);
            if(bottom < 0) {
                tf->translate(vec_up * (-bottom));
                if(reg.has_components<velocity>(en)) {
                    auto vel = reg.get_components<velocity>(en);
                    vel->value.y = std::max(0.F, vel->value.y);
                }
                if(reg.has_components<jumpable>(en)) {
                    if(reg.has_components<jumped>(en)) { reg.remove_component<jumped>(en); }
                    if(!reg.has_components<can_jump>(en)) {
                        reg.add_component<const can_jump>(en);
                    }
                }
            }
        }
    }
};

class jump_system {
public:
    static void update(seconds, tree_context &ctx) {
        auto &window = ctx.ecs().get_context<runtime_info>().window();
        if(is_jump_button_pressed(window)) {
            jump_all(ctx.ecs());
        }
    }

private:
    static void jump_all(ecs_registry &reg) {
        for(auto [en, vel, jump]: reg.each<velocity, const jumpable>()) {
            if(reg.has_components<can_jump>(en)) {
                vel->value.y = std::sqrt(std::abs(2.F * jump->height * data::gravity.y));
                reg.remove_component<can_jump>(en);
                if(!reg.has_components<jumped>(en)) {
                    reg.add_component<jumped>(en);
                }
            }
        }
    }
};

class dino_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, jumped>().connect<&dino_system::to_jump_state>();
        reg.on<comp_event::construct, can_jump>().connect<&dino_system::to_run_state>();
    }
    static void update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &window = reg.get_context<runtime_info>().window();
        const auto duck_button_pressed = is_duck_button_pressed(window);

        for(auto [en, dino]: reg.each<dino_state>()) {
            switch(*dino) {
            case dino_state::run: {
                if(!duck_button_pressed) { break; }
                *dino = dino_state::run_duck;
                auto anim = reg.get_components<animation>(en);
                to_duck_animation(*anim);
                break;
            }
            case dino_state::run_duck:
                if(duck_button_pressed) { break; }
                to_run_state(reg, en);
            default:
                break;
            }
        }
    }

private:
    static void to_jump_state(ecs_registry &reg, entity en) {
        if(!reg.has_components<dino_state>(en)) { return; }
        {
            auto state = reg.get_components<dino_state>(en);
            *state = dino_state::jump;
        }
        if(reg.has_components<animation>(en)) {
            auto anim = reg.get_components<animation>(en);
            anim->frames = {data::rects.at(data::sprite::dino_jump)};
        }
    }
    static void to_run_state(ecs_registry &reg, entity en) {
        if(!reg.has_components<dino_state>(en)) { return; }
        {
            auto state = reg.get_components<dino_state>(en);
            *state = dino_state::run;
        }
        if(reg.has_components<animation>(en)) {
            auto anim = reg.get_components<animation>(en);
            anim->frames = data::dino_run_frames;
        }
    }
    static void to_duck_animation(animation &anim) {
        anim.frames = data::dino_run_duck_frames;
    }
};

class game_system {
public:
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();

        auto &res = ctx.root().add_child();
        auto &scene = ctx.root().add_child();

        const auto material_holder = res.entities().create();
        const auto &texture = *reg.add_component<const texture_2d_data>(material_holder, "assets/base64.png");
        reg.add_component<const material_data>(material_holder, material_data{.texture_holder = material_holder});

        m_mesh_holders = create_meshes(reg, res, texture);

        auto cam = scene.entities().create();
        reg.add_component<const camera>(cam, camera{.clear_color = vec4f{1.F}});
        reg.add_component<const main_camera>(cam);
        auto cam_tf = reg.get_components<transform>(cam);
        cam_tf->set_position(vec_back * 3.F + 1.5F * vec_up);

        m_dino = scene.entities().create();
        reg.add_component<const rendered_mesh>(
            m_dino,
            rendered_mesh{
                .mesh_holder = m_mesh_holders[data::sprite::dino_wait],
                .material_holder = material_holder,
            });
        reg.add_component<const dino_state>(m_dino, dino_state::idle);
        auto dino_tf = reg.get_components<transform>(m_dino);
        dino_tf->translate(4.F * vec_left + vec_up * data::dino_bounding_box.y / 2.F);
    }

    void input(const event &ev, tree_context &ctx) {
        switch(m_current_state) {
        case state::idle:
            if(const auto *key = ev.try_get<event::key_pressed>(); key != nullptr && is_jump_button(key->code)) {
                to_transition(ctx.ecs());
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

    void to_transition(ecs_registry &reg) {
        m_current_state = state::transition;
        reg.add_component<const velocity>(m_dino);
        reg.add_component<const jumpable>(m_dino, jumpable{.height = data::dino_jump_height});
        reg.add_component<const bounding_box>(m_dino, vec2f{1.F, 1.F});
        reg.add_component<const animation>(m_dino, animation{.frames = data::dino_run_frames, .fps = data::dino_fps});
    }

    state m_current_state{state::idle};
    std::unordered_map<data::sprite, entity> m_mesh_holders;
    entity m_dino;
};

int main() {
    constexpr vec2u win_size{800u, 300u};
    constexpr auto updates_per_sec = 120;
    try {
        app my_app{{
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
        my_app.enable_default_systems();
        my_app
            .systems()
            .add<game_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::input>();
        my_app
            .systems()
            .add<movement_system>()
            .run_as<sys_type::update>(sys_priority::high);
        my_app
            .systems()
            .add<jump_system>()
            .run_as<sys_type::update>();
        my_app
            .systems()
            .add<bound_system>()
            .run_as<sys_type::post_update>();
        my_app
            .systems()
            .add<animation_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::post_update>();
        my_app
            .systems()
            .add<dino_system>()
            .run_as<sys_type::update>()
            .run_as<sys_type::start>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

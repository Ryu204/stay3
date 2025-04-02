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
    reg.emplace<mesh_data>(en, mesh_sprite(texture, data::pixels_per_unit, vec4f{1.F}, data::rects.at(id)));
}

std::unordered_map<data::sprite, entity> create_meshes(ecs_registry &reg, node &resource_node, const texture_2d_data &texture) {
    std::unordered_map<data::sprite, entity> result;
    for(auto spr: {data::sprite::dino_wait, data::sprite::ground}) {
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
    vec3f accel{data::gravity};
};
struct jumpable {
    float height{};
};
struct can_jump {};
struct jumped {};
struct bounding_box {
    vec2f size;
};
struct bounding_box_debug {
    entity renderer;
};

struct animation {
    std::vector<rectf> frames;
    float fps{};
};
struct animation_status {
    decltype(animation::frames)::size_type index{};
    float current_time{};
};
enum class ground_status : std::uint8_t {
    started,
    scrolling,
};
using mesh_holders = std::unordered_map<data::sprite, entity>;
struct texture_holder {
    entity holder;
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
        for(auto [en, tf, vel]: ctx.ecs().each<mut<transform>, mut<velocity>>()) {
            vel->value += vel->accel * delta;
            tf->translate(vel->value * delta);
        }
    }
};

class animation_system {
    static void update_anim_frame(ecs_registry &reg, const animation &anim, animation_status &anim_status, const rendered_mesh &rmesh) {
        const auto &texture = *rmesh.material.get(reg)->texture.get(reg);
        auto mesh = rmesh.mesh.get_mut(reg);
        anim_status.index %= anim.frames.size();
        *mesh = mesh_sprite(texture, data::pixels_per_unit, vec4f{1.F}, anim.frames[anim_status.index]);
    }

public:
    static void start(tree_context &ctx) {
        ctx.ecs().on<comp_event::update, animation>().connect<+[](ecs_registry &reg, entity en) {
            update_anim_frame(
                reg,
                *reg.get<animation>(en),
                *reg.get<mut<animation_status>>(en),
                *reg.get<rendered_mesh>(en));
        }>();
        make_hard_dependency<animation_status, animation>(ctx.ecs());
    }
    static void post_update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, anim, anim_status, rmesh]: reg.each<animation, mut<animation_status>, rendered_mesh>()) {
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
        for(auto [en, tf, box]: reg.each<mut<transform>, bounding_box>()) {
            const auto bottom = tf->position().y - (box->size.y / 2.F);
            if(bottom < 0) {
                tf->translate(vec_up * (-bottom));
                if(reg.contains<velocity>(en)) {
                    auto vel = reg.get<mut<velocity>>(en);
                    vel->value.y = std::max(0.F, vel->value.y);
                }
                if(reg.contains<jumpable>(en)) {
                    reg.destroy_if_exist<jumped>(en);
                    reg.emplace_if_not_exist<can_jump>(en);
                }
            }
        }
    }
};

class bound_debug_system {
    static void add_shape_entity(tree_context &ctx, ecs_registry &reg, entity en) {
        const auto box = reg.get<bounding_box>(en);
        auto db = reg.emplace<mut<bounding_box_debug>>(en);
        db->renderer = ctx.get_node(en).entities().create();
        reg.emplace<mesh_data>(db->renderer, mesh_plane(box->size, vec4f{0.F, 1.F, 0.F, 1.F}));
        reg.emplace<material_data>(db->renderer);
        reg.emplace<rendered_mesh>(db->renderer, rendered_mesh{.mesh = db->renderer, .material = db->renderer});
    }
    static void remove_shape_entity(tree_context &ctx, ecs_registry &reg, entity en) {
        const auto db_entity = reg.get<bounding_box_debug>(en)->renderer;
        if(reg.contains(db_entity)) {
            ctx.get_node(en).entities().destroy(db_entity);
        }
    }

public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::construct, bounding_box>().connect<&bound_debug_system::add_shape_entity>(ctx);
        reg.on<comp_event::destroy, bounding_box_debug>().connect<&bound_debug_system::remove_shape_entity>(ctx);
    }
    static void update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, db, tf]: reg.each<bounding_box_debug, transform>()) {
            auto renderer_tf = reg.get<mut<transform>>(db->renderer);
            *renderer_tf = *tf;
        }
    }
};

class jump_system {
public:
    static void update(seconds, tree_context &ctx) {
        auto &window = ctx.vars().get<runtime_info>().window();
        if(is_jump_button_pressed(window)) {
            jump_all(ctx.ecs());
        }
    }

private:
    static void jump_all(ecs_registry &reg) {
        for(auto [en, vel, jump]: reg.each<mut<velocity>, jumpable>()) {
            if(reg.contains<can_jump>(en)) {
                vel->value.y = std::sqrt(std::abs(2.F * jump->height * data::gravity.y));
                reg.destroy<can_jump>(en);
                if(!reg.contains<jumped>(en)) {
                    reg.emplace<jumped>(en);
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
        auto &window = ctx.vars().get<runtime_info>().window();
        const auto duck_button_pressed = is_duck_button_pressed(window);

        for(auto [en, dino]: reg.each<mut<dino_state>>()) {
            switch(*dino) {
            case dino_state::run: {
                if(!duck_button_pressed) { break; }
                *dino = dino_state::run_duck;
                auto anim = reg.get<mut<animation>>(en);
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
        if(!reg.contains<dino_state>(en)) { return; }
        {
            auto state = reg.get<mut<dino_state>>(en);
            *state = dino_state::jump;
        }
        if(reg.contains<animation>(en)) {
            auto anim = reg.get<mut<animation>>(en);
            anim->frames = {data::rects.at(data::sprite::dino_jump)};
        }
    }
    static void to_run_state(ecs_registry &reg, entity en) {
        if(!reg.contains<dino_state>(en)) { return; }
        {
            auto state = reg.get<mut<dino_state>>(en);
            *state = dino_state::run;
        }
        if(reg.contains<animation>(en)) {
            auto anim = reg.get<mut<animation>>(en);
            anim->frames = data::dino_run_frames;
        }
    }
    static void to_duck_animation(animation &anim) {
        anim.frames = data::dino_run_duck_frames;
    }
};

class ground_system {
    struct ground_hider {};

    static void on_change(tree_context &ctx, ecs_registry &reg, entity en) {
        auto status = *reg.get<ground_status>(en);
        if(status == ground_status::started) {
            const auto &meshes = ctx.vars().get<mesh_holders>();
            const auto texture = ctx.vars().get<texture_holder>().holder;
            reg.emplace<material_data>(en, material_data{.texture = texture});
            reg.emplace<rendered_mesh>(
                en,
                rendered_mesh{
                    .mesh = meshes.at(data::sprite::ground),
                    .material = en,
                });
            auto ground_tf = reg.get<mut<transform>>(en);
            ground_tf
                ->translate(vec_up * (data::rects.at(data::sprite::ground).size.y / data::pixels_per_unit / 2.F))
                .translate(vec_forward * 0.0001F)
                .translate(vec_right * (data::rects.at(data::sprite::ground).size.x / data::pixels_per_unit / 2.F))
                .translate(vec_left * 4.F)
                .translate(vec_left * (data::rects.at(data::sprite::dino_wait).size.x / data::pixels_per_unit / 2.F));

            auto hider = ctx.root().add_child().entities().create();
            reg.emplace<ground_hider>(hider);
            reg.emplace<mesh_data>(hider, mesh_plane(vec2f{15.F}, vec4f{1.F}));
            reg.emplace<material_data>(hider);
            reg.emplace<rendered_mesh>(hider, rendered_mesh{.mesh = hider, .material = hider});
            auto hider_tf = reg.get<mut<transform>>(hider);
            hider_tf->set_position(vec3f{4.F, 0.F, -0.1F});
            reg.emplace<velocity>(hider, velocity{.value = 10.F * vec_right, .accel = {}});
        }
    }

public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        reg.on<comp_event::update, ground_status>().connect<&ground_system::on_change>(ctx);
    }
    static void update(seconds, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, gr]: reg.each<ground_status>()) {
            switch(*gr) {
            case ground_status::started: {
                break;
            }
            case ground_status::scrolling: {
            }
            default:
                break;
            }
        }
        for(auto [en, hider, tf]: reg.each<ground_hider, transform>()) {
            if(tf->position().x >= 11.5F) {
                reg.destroy_if_exist<velocity>(en);
            }
        }
    }
};

class game_system {
public:
    void start(tree_context &ctx) {
        auto &reg = ctx.ecs();

        auto &res = ctx.root().add_child();
        auto &scene = ctx.root().add_child();

        auto texture_en = res.entities().create();
        const auto &texture = *reg.emplace<texture_2d_data>(texture_en, "assets/base64.png");
        ctx.vars().emplace<texture_holder>(texture_holder{.holder = texture_en});
        auto material = res.entities().create();
        reg.emplace<material_data>(material, material_data{.texture = texture_en});
        const auto &meshes = ctx.vars().emplace<mesh_holders>(create_meshes(reg, res, texture));

        auto cam = scene.entities().create();
        reg.emplace<camera>(cam, camera{.clear_color = vec4f{1.F}});
        reg.emplace<main_camera>(cam);
        auto cam_tf = reg.get<mut<transform>>(cam);
        cam_tf->set_position(vec_back * 3.F + 1.5F * vec_up);

        m_dino = scene.entities().create();
        reg.emplace<rendered_mesh>(
            m_dino,
            rendered_mesh{
                .mesh = meshes.at(data::sprite::dino_wait),
                .material = material,
            });
        reg.emplace<dino_state>(m_dino, dino_state::idle);
        auto dino_tf = reg.get<mut<transform>>(m_dino);
        dino_tf->translate(4.F * vec_left + vec_up * data::dino_bounding_box.y / 2.F);

        m_ground = scene.entities().create();
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
        reg.emplace<velocity>(m_dino);
        reg.emplace<jumpable>(m_dino, jumpable{.height = data::dino_jump_height});
        reg.emplace<bounding_box>(m_dino, data::dino_bounding_box);
        reg.emplace<animation>(m_dino, animation{.frames = data::dino_run_frames, .fps = data::dino_fps});

        reg.emplace<mut<ground_status>>(m_ground, ground_status::started);
    }

    state m_current_state{state::idle};
    entity m_dino;
    entity m_ground;
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
            .add<bound_debug_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
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
        my_app
            .systems()
            .add<ground_system>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

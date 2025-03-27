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
};
const std::unordered_map<sprite, rectf> rects = {
    {sprite::dino_wait, {.position = {76.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_1, {.position = {1854.F, 2.F}, .size = {88.F, 94.F}}},
    {sprite::dino_run_2, {.position = {1942.F, 2.F}, .size = {88.F, 94.F}}},
};
constexpr auto pixels_per_unit = 100.F;
constexpr vec3f gravity{0.F, -20.F, 0.F};
constexpr auto dino_jump_height = 1.5F;
const vec2f dino_bounding_box = vec2f{40.F, 94.F} / pixels_per_unit;
const std::vector dino_run_frames = {
    rects.at(sprite::dino_run_1),
    rects.at(sprite::dino_run_2),
};
constexpr auto dino_fps = 6.F;
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
    return code == scancode::enter || code == scancode::space;
}

struct velocity {
    vec3f value;
};
struct jumpable {
    float height{};
    bool can_jump{true};
};
struct bounding_box {
    vec2f size;
};

struct animation {
    std::vector<rectf> frames;
    float fps{};

    decltype(frames)::size_type index{};
    float current_time{};
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
public:
    static void post_update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto [en, anim, rmesh]: reg.each<animation, const rendered_mesh>()) {
            anim->current_time += delta;
            bool changed{false};
            while(anim->current_time > 1.F / anim->fps) {
                anim->current_time -= 1.F / anim->fps;
                ++anim->index;
                anim->index %= anim->frames.size();
                changed = true;
            }
            if(!changed) { return; }
            auto mesh = reg.get_components<mesh_data>(rmesh->mesh_holder);
            const auto texture_holder = reg.get_components<const material_data>(rmesh->material_holder)->texture_holder;
            const auto &texture = *reg.get_components<const texture_2d_data>(texture_holder);
            *mesh = mesh_sprite(texture, data::pixels_per_unit, vec4f{1.F}, anim->frames[anim->index]);
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
                    auto jump = reg.get_components<jumpable>(en);
                    jump->can_jump = true;
                }
            }
        }
    }
};

class jump_system {
public:
    static void input(const event &ev, tree_context &ctx) {
        if(ev.is<event::key_pressed>() && is_jump_button(ev.try_get<event::key_pressed>()->code)) {
            jump_all(ctx.ecs());
        }
    }
    static void update(seconds, tree_context &ctx) {
        auto &window = ctx.ecs().get_context<runtime_info>().window();
        if(window.get_key(scancode::enter) == key_status::pressed || window.get_key(scancode::space) == key_status::pressed) {
            jump_all(ctx.ecs());
        }
    }

private:
    static void jump_all(ecs_registry &reg) {
        for(auto [en, vel, jump]: reg.each<velocity, jumpable>()) {
            if(jump->can_jump) {
                vel->value.y = std::sqrt(std::abs(2.F * jump->height * data::gravity.y));
                jump->can_jump = false;
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

        const auto material_holder = res.entities().create();
        const auto &texture = *reg.add_component<const texture_2d_data>(material_holder, "assets/base64.png");
        reg.add_component<const material_data>(material_holder, material_data{.texture_holder = material_holder});

        m_mesh_holders = create_meshes(reg, res, texture);

        auto cam = scene.entities().create();
        reg.add_component<const camera>(cam, camera{.clear_color = vec4f{0.F}});
        reg.add_component<const main_camera>(cam);
        auto cam_tf = reg.get_components<transform>(cam);
        cam_tf->set_position(vec_back * 3.F + vec_up);

        m_dino = scene.entities().create();
        reg.add_component<const rendered_mesh>(
            m_dino,
            rendered_mesh{
                .mesh_holder = m_mesh_holders[data::sprite::dino_wait],
                .material_holder = material_holder,
            });
        auto dino_tf = reg.get_components<transform>(m_dino);
        dino_tf->translate(3.F * vec_left);
    }

    void input(const event &ev, tree_context &ctx) {
        switch(m_current_state) {
        case state::idle:
            if(const auto *key = ev.try_get<event::key_pressed>(); key != nullptr) {
                if(is_jump_button(key->code)) {
                    to_transition(ctx.ecs());
                }
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
    constexpr vec2u win_size{800u, 400u};
    constexpr auto updates_per_sec = 120;
    try {
        app my_app{{
            .window = {
                .size = win_size,
                .name = "Dino game",
            },
            .updates_per_second = updates_per_sec,
            .render = {.power_pref = render_config::power_preference::low},
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
            .run_as<sys_type::input>()
            .run_as<sys_type::update>();
        my_app
            .systems()
            .add<bound_system>()
            .run_as<sys_type::post_update>();
        my_app
            .systems()
            .add<animation_system>()
            .run_as<sys_type::post_update>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

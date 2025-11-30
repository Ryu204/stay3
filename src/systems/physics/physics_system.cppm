module;

#include <cassert>
#include <memory>
#include <ranges>
#include <vector>
// Needed for `ecs::registry::get` structured binding
#include <tuple>
// Needed for `std::unordered_set` ranged for loop
#include <unordered_set>

export module stay3.system.physics;

import stay3.node;
import stay3.ecs;
import stay3.core;
import stay3.physics;
import stay3.system.transform;

namespace st {
/** @brief Tag to prevent transform changes from changing physics state of an entity */
struct no_update_physics_state {};
/** @brief Tag for entities of which physics state needs to be updated from transform */
struct update_physics_state_from_transform {};

export class physics_system {
public:
    physics_system(const physics_config &settings = {})
        : m_config{settings} {}

    void start(tree_context &ctx) {
        m_world = std::make_unique<physics_world>(ctx, m_config);
        setup_signals(ctx);
        log::info("Physics system started");
    }

    void update(seconds delta, tree_context &ctx) {
        auto &reg = ctx.ecs();
        // We need to copy the view to fixed vector because iteration may invalidate it
        const auto bodies_need_change = std::ranges::to<std::vector<entity>>(
            reg.view<update_physics_state_from_transform>());
        for(auto en: bodies_need_change) {
            reg.emplace<no_update_physics_state>(en);
            const auto &id = *reg.get<physics_world::body_id>(en);
            const auto &global_tf = sync_global_transform(ctx, en).get();
            m_world->set_transform(id, global_tf.position(), global_tf.orientation());
            reg.destroy<update_physics_state_from_transform>(en);
        }
        reg.destroy_all<no_update_physics_state>();

        for(auto [en, collection]: reg.each<mut<collision_enter>>()) {
            collection->clear();
        }
        for(auto [en, collection]: reg.each<mut<collision_exit>>()) {
            collection->clear();
        }
        for(auto [en, collection]: reg.each<mut<collision_stay>>()) {
            collection->clear();
        }
        m_world->update(delta);

        for(const auto &id: m_world->bodies_with_changed_state()) {
            auto en = m_world->entity(id);
            reg.emplace<no_update_physics_state>(en);
            set_global_transform(ctx, en, m_world->transform(id));
        }
        reg.destroy_all<no_update_physics_state>();
    }

    void render(tree_context &) {
        m_world->render();
    }

private:
    // This struct is to get around 1 extra argument limitation of event handler
    struct on_collider_construct_args {
        tree_context *ctx{};
        physics_system *system{};
    };
    on_collider_construct_args m_on_collider_construct_args;

    void setup_signals(tree_context &ctx) {
        auto &reg = ctx.ecs();
        // Rigidbody and collider
        make_soft_dependency<transform, rigidbody>(reg);
        m_on_collider_construct_args = {.ctx = &ctx, .system = this};
        reg.on<comp_event::construct, collider>()
            .connect<&physics_system::on_collider_construct>(m_on_collider_construct_args);
        reg.on<comp_event::destroy, collider>().connect<&physics_system::on_collider_destroy>();
        reg.on<comp_event::destroy, physics_world::body_id>().connect<&physics_system::on_body_id_destroy>(*this);
        reg.on<comp_event::update, global_transform>().connect<&physics_system::on_actual_global_transform_update>();
        reg.on<comp_event::update, transform>().connect<&physics_system::on_actual_global_transform_update>();
        reg.on<comp_event::update, rigidbody>().connect<+[](ecs_registry &, entity) {
            assert(false && "Rigidbody changes are not supported");
        }>();
        reg.on<comp_event::update, collider>().connect<+[](ecs_registry &, entity) {
            assert(false && "Collider changes are not supported");
        }>();
        // Collision info
        make_hard_dependency<collision_exit_unprocessed, collision_exit>(reg);
    }

    static void on_collider_construct(on_collider_construct_args &args, ecs_registry &reg, entity en) {
        assert(reg.contains<rigidbody>(en) && "Add rigidbody before collider");
        const auto &global_tf = sync_global_transform(*args.ctx, en).get();
        auto col = reg.get<collider>(en);
        const auto &rg = *reg.get<rigidbody>(en);
        const auto body_id = args.system->m_world->create(*col, global_tf.position(), global_tf.orientation(), rg, en);
        reg.emplace<physics_world::body_id>(en, body_id);

        switch(rg.motion_type) {
        case rigidbody::type::fixed:
            break;
        case rigidbody::type::kinematic:
            assert(false && "Unimplemented");
            break;
        case rigidbody::type::dynamic:
            reg.emplace<motion>(en, *args.system->m_world, body_id);
            break;
        default:
            assert(false && "Invalid value");
        }
    }

    static void on_collider_destroy(ecs_registry &reg, entity en) {
        reg.destroy_if_exist<physics_world::body_id>(en);
        reg.destroy_if_exist<motion>(en);
        reg.destroy_if_exist<collision_exit>(en);
        reg.destroy_if_exist<collision_enter>(en);
        reg.destroy_if_exist<collision_stay>(en);
    }

    void on_body_id_destroy(ecs_registry &reg, entity en) {
        m_world->destroy(*reg.get<physics_world::body_id>(en));
    }

    static void on_actual_global_transform_update(ecs_registry &reg, entity en) {
        if(reg.contains<physics_world::body_id>(en) && !reg.contains<no_update_physics_state>(en)) {
            reg.emplace_if_not_exist<update_physics_state_from_transform>(en);
        }
    }

    physics_config m_config;
    std::unique_ptr<physics_world> m_world;
};
} // namespace st
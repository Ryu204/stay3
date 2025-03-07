module;

#include <cstdint>
#include <unordered_map>

module stay3.system.transform;

import stay3.node;
import stay3.node;
import stay3.ecs;

struct dirty_flag {};

namespace st {

void add_dirty_flag(st::ecs_registry &reg, st::entity en) {
    if(!reg.has_components<dirty_flag>(en)) {
        reg.add_component<dirty_flag>(en);
    }
}

const transform &global_transform::get() const {
    return global;
}

void transform_sync_system::start(tree_context &ctx) {
    auto &reg = ctx.ecs();

    reg.on<comp_event::construct, transform>().connect<&ecs_registry::add_component<global_transform>>();
    reg.on<comp_event::destroy, transform>().connect<&ecs_registry::remove_component<global_transform>>();

    reg.on<comp_event::update, transform>().connect<&add_dirty_flag>();
    // New transform is dirty by default
    reg.on<comp_event::construct, transform>().connect<&add_dirty_flag>();
}

void transform_sync_system::post_update(seconds, tree_context &ctx) {
    sync_global_transform(ctx);
}

void sync_global_transform(tree_context &ctx) {
    std::unordered_map<node::id_type, std::uint32_t> ancestor_count;

    ctx.root().traverse(
        [&ancestor_count](const node &cur, std::uint32_t count) {
            ancestor_count.emplace(cur.id(), count);
            return count + 1;
        },
        0);

    auto &reg = ctx.ecs();
    reg.sort<dirty_flag>([&ctx, &ancestor_count](entity prev, entity later) {
        return ancestor_count[ctx.get_node(prev).id()] < ancestor_count[ctx.get_node(later).id()];
    });

    for(auto &&[en, unused]: reg.each<const dirty_flag>()) {
        const auto &node = ctx.get_node(en);
        const auto local = reg.get_component<const transform>(en);
        auto global = reg.get_component<global_transform>(en);
        const auto has_parent_transform =
            !node.is_root()
            && !node.parent().entities().is_empty()
            && reg.has_components<global_transform>(node.parent().entities()[0]);
        if(!has_parent_transform) {
            global->global = *local;
            continue;
        }
        auto parent_global = reg.get_component<const global_transform>(node.parent().entities()[0]);
        global->global.set_matrix(parent_global->global.matrix() * local->matrix());
    }
    reg.clear_component<dirty_flag>();
}

global_transform &sync_global_transform(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    auto &&target = reg.get_component<global_transform>(en);
    //     if(!target.dirty) { goto RETURN; }

    //     const auto &node = ctx.get_node(en);
    //     const auto *parent = node.parent();
    //     const auto is_root_node = parent == nullptr;
    //     if(is_root_node) { goto RETURN; }

    //     const auto has_parent_transform = !parent->entities().empty()
    //                                       && reg.has_components<global_transform>(parent->entities()[0]);
    //     if(!has_parent_transform) { goto RETURN; }

    //     const auto &parent_transform = sync_global_transform(ctx, parent->entities()[0]);
    //     const auto &local_transform = reg.get_component<transform>(en);
    //     target.global.set_matrix(parent_transform.matrix() * local_transform.matrix());

    // RETURN:
    //     target.dirty = false;
    return *target;
}

} // namespace st
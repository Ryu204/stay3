module stay3.system.transform;

import stay3.node;
import stay3.node;
import stay3.ecs;

struct dirty_flag {};

void add_dirty_flag(st::ecs_registry &reg, st::entity en) {
    if(!reg.has_components<dirty_flag>(en)) {
        reg.add_component<dirty_flag>(en);
    }
}

namespace st {

const transform &global_transform::get() const {
    return global;
}

void transform_sync_system::start(tree_context &ctx) {
    ctx.ecs().on<comp_event::construct, transform>().connect<&add_dirty_flag>();
}

void transform_sync_system::post_update(tree_context &ctx) {
}

void sync_global_transform(tree_context &ctx) {
    auto &reg = ctx.ecs();

    // Sort the global transform order so parent is before children
    reg.sort<global_transform>([&ctx](entity lhs, entity rhs) {
        const auto lhs_node_id = ctx.get_node(lhs).id();
        const auto rhs_node_id = ctx.get_node(rhs).id();
        // TODO: impl
        return true;
    });
}

// global_transform &sync_global_transform(tree_context &ctx, entity en) {
// auto &reg = ctx.ecs();
// auto &&target = reg.get_component<global_transform>(en);
// //     if(!target.dirty) { goto RETURN; }

// //     const auto &node = ctx.get_node(en);
// //     const auto *parent = node.parent();
// //     const auto is_root_node = parent == nullptr;
// //     if(is_root_node) { goto RETURN; }

// //     const auto has_parent_transform = !parent->entities().empty()
// //                                       && reg.has_components<global_transform>(parent->entities()[0]);
// //     if(!has_parent_transform) { goto RETURN; }

// //     const auto &parent_transform = sync_global_transform(ctx, parent->entities()[0]);
// //     const auto &local_transform = reg.get_component<transform>(en);
// //     target.global.set_matrix(parent_transform.matrix() * local_transform.matrix());

// // RETURN:
// //     target.dirty = false;
// return target;
// }

} // namespace st
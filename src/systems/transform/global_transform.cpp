module;

#include <cassert>
#include <cstdint>
#include <unordered_map>

module stay3.system.transform;

import stay3.node;
import stay3.core;
import stay3.ecs;

struct dirty_flag {};

namespace st {

void mark_subtree_dirty(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    assert(reg.contains<transform>(en));
    const auto &node = ctx.get_node(en);

    if(node.entities()[0] != en) {
        reg.emplace_if_not_exist<dirty_flag>(en);
        return;
    }
    if(reg.contains<dirty_flag>(en)) {
        return;
    }

    constexpr auto children_needs_mark = [](ecs_registry &reg, const class node &node) {
        if(node.entities().is_empty()) { return false; }
        return reg.contains<transform>(node.entities()[0])
               && !reg.contains<dirty_flag>(node.entities()[0]);
    };
    constexpr auto traverse_tree = [children_needs_mark](auto &&self, ecs_registry &reg, const class node &node) -> void {
        const auto will_mark_children = children_needs_mark(reg, node);
        for(auto en: node.entities()) {
            if(reg.contains<transform>(en)) {
                reg.emplace_if_not_exist<dirty_flag>(en);
            }
        }
        if(will_mark_children) {
            for(const auto &child_node: node) {
                self(self, reg, child_node);
            }
        };
    };
    reg.emplace<dirty_flag>(en);
    for(const auto &child: node) {
        traverse_tree(traverse_tree, reg, child);
    }
}

void mark_subtree_dirty_except_root(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    for(const auto &child: ctx.get_node(en)) {
        for(const auto child_en: child.entities()) {
            if(reg.contains<transform>(child_en)) {
                mark_subtree_dirty(ctx, child_en);
            }
        }
    }
}

void node_reparented_handler(tree_context &ctx, tree_context::node_reparented_args args) {
    const auto &node = ctx.get_node(args.current);
    for(auto en: node.entities()) {
        if(ctx.ecs().contains<transform>(en)) {
            mark_subtree_dirty(ctx, en);
        }
    }
}

void transform_constructed_handler(tree_context &ctx, ecs_registry &, entity en) {
    auto &reg = ctx.ecs();
    reg.emplace<global_transform>(en);
    mark_subtree_dirty(ctx, en);
}

void transform_destroyed_handler(tree_context &ctx, ecs_registry &, entity en) {
    auto &reg = ctx.ecs();
    if(en == ctx.get_node(en).entities()[0]) {
        mark_subtree_dirty_except_root(ctx, en);
    }
    reg.destroy_if_exist<global_transform>(en);
    reg.destroy_if_exist<dirty_flag>(en);
}

void transform_updated_handler(tree_context &ctx, ecs_registry &, entity en) {
    mark_subtree_dirty(ctx, en);
}

const transform &global_transform::get() const {
    return global;
}

void transform_sync_system::start(tree_context &ctx) {
    auto &reg = ctx.ecs();

    reg.on<comp_event::construct, transform>().connect<&transform_constructed_handler>(ctx);
    reg.on<comp_event::destroy, transform>().connect<&transform_destroyed_handler>(ctx);
    reg.on<comp_event::update, transform>().connect<&transform_updated_handler>(ctx);

    ctx.on_node_reparented().connect<&node_reparented_handler>(ctx);
    log::info("Transform sync system started");
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

    for(auto &&[en, unused]: reg.each<dirty_flag>()) {
        const auto &node = ctx.get_node(en);
        if(!reg.contains<transform>(en)) {
            continue;
        }
        auto [local, global] = reg.get<transform, mut<global_transform>>(en);
        const auto has_parent_transform =
            !node.is_root()
            && !node.parent().entities().is_empty()
            && reg.contains<global_transform>(node.parent().entities()[0]);
        if(!has_parent_transform) {
            global->global = *local;
            continue;
        }
        auto parent_global = reg.get<global_transform>(node.parent().entities()[0]);
        global->global.set_matrix(parent_global->global.matrix() * local->matrix());
    }
    reg.destroy_all<dirty_flag>();
}

const global_transform &sync_global_transform(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    if(!reg.contains<dirty_flag>(en)) {
        return *reg.get<global_transform>(en);
    }
    const auto &node = ctx.get_node(en);
    const auto has_parent_transform =
        !node.is_root()
        && !node.parent().entities().is_empty()
        && reg.contains<global_transform>(node.parent().entities()[0]);
    auto [local, global] = reg.get<transform, mut<global_transform>>(en);
    if(!has_parent_transform) {
        global->global = *local;
    } else {
        auto parent_en = node.parent().entities()[0];
        const auto &parent_global = sync_global_transform(ctx, parent_en);
        global->global.set_matrix(parent_global.global.matrix() * local->matrix());
    }
    reg.destroy<dirty_flag>(en);
    return *reg.get<global_transform>(en);
}

void set_global_transform(tree_context &ctx, entity en, const transform &value) {
    auto &reg = ctx.ecs();
    assert(reg.contains<transform>(en));
    const auto &my_node = ctx.get_node(en);
    const auto parent_has_tf =
        !my_node.is_root()
        && !my_node.parent().entities().is_empty()
        && reg.contains<transform>(my_node.parent().entities()[0]);
    const auto is_independent = !parent_has_tf || en != my_node.entities()[0];
    if(is_independent) {
        *reg.get<mut<transform>>(en) = value;
        reg.get<mut<global_transform>>(en)->global = value;
        reg.destroy_if_exist<dirty_flag>(en);
        return;
    }
    const auto &parent_tf = sync_global_transform(ctx, my_node.parent().entities()[0]).get();
    reg.get<mut<transform>>(en)->set_matrix(parent_tf.inv_matrix() * value.matrix());
    reg.get<mut<global_transform>>(en)->global = value;
    reg.destroy_if_exist<dirty_flag>(en);
}

} // namespace st
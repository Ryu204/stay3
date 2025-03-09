module;

#include <cassert>
#include <cstdint>
#include <unordered_map>

module stay3.system.transform;

import stay3.node;
import stay3.node;
import stay3.ecs;

struct dirty_flag {};

namespace st {

void mark_entity_dirty(ecs_registry &reg, entity en) {
    if(!reg.has_components<dirty_flag>(en)) {
        reg.add_component<const dirty_flag>(en);
    }
}

void mark_subtree_dirty(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    assert(reg.has_components<transform>(en));
    const auto &node = ctx.get_node(en);

    if(node.entities()[0] != en) {
        mark_entity_dirty(reg, en);
        return;
    }
    if(reg.has_components<dirty_flag>(en)) {
        return;
    }

    constexpr auto children_needs_mark = [](ecs_registry &reg, const class node &node) {
        if(node.entities().is_empty()) { return false; }
        return reg.has_components<transform>(node.entities()[0])
               && !reg.has_components<dirty_flag>(node.entities()[0]);
    };
    constexpr auto traverse_tree = [children_needs_mark](auto &&self, ecs_registry &reg, const class node &node) -> void {
        const auto will_mark_children = children_needs_mark(reg, node);
        for(auto en: node.entities()) {
            if(reg.has_components<transform>(en)) {
                mark_entity_dirty(reg, en);
            }
        }
        if(will_mark_children) {
            for(const auto &child_node: node) {
                self(self, reg, child_node);
            }
        };
    };
    reg.add_component<dirty_flag>(en);
    for(const auto &child: node) {
        traverse_tree(traverse_tree, reg, child);
    }
}

void mark_subtree_dirty_except_root(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    for(const auto &child: ctx.get_node(en)) {
        for(const auto child_en: child.entities()) {
            if(reg.has_components<transform>(child_en)) {
                mark_subtree_dirty(ctx, child_en);
            }
        }
    }
}

void node_reparented_handler(tree_context &ctx, tree_context::node_reparented_args args) {
    const auto &node = ctx.get_node(args.current);
    for(auto en: node.entities()) {
        if(ctx.ecs().has_components<transform>(en)) {
            mark_subtree_dirty(ctx, en);
        }
    }
}

void transform_constructed_handler(tree_context &ctx, ecs_registry &, entity en) {
    auto &reg = ctx.ecs();
    reg.add_component<const global_transform>(en);
    mark_subtree_dirty(ctx, en);
}

void transform_destroyed_handler(tree_context &ctx, ecs_registry &, entity en) {
    auto &reg = ctx.ecs();
    if(en == ctx.get_node(en).entities()[0]) {
        mark_subtree_dirty_except_root(ctx, en);
    }
    reg.remove_component<global_transform>(en);
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
        if(!reg.has_components<transform>(en)) {
            continue;
        }
        auto [local, global] = reg.get_components<const transform, global_transform>(en);
        const auto has_parent_transform =
            !node.is_root()
            && !node.parent().entities().is_empty()
            && reg.has_components<global_transform>(node.parent().entities()[0]);
        if(!has_parent_transform) {
            global->global = *local;
            continue;
        }
        auto parent_global = reg.get_components<const global_transform>(node.parent().entities()[0]);
        global->global.set_matrix(parent_global->global.matrix() * local->matrix());
    }
    reg.clear_component<dirty_flag>();
}

const global_transform &sync_global_transform(tree_context &ctx, entity en) {
    auto &reg = ctx.ecs();
    if(!reg.has_components<dirty_flag>(en)) {
        return *reg.get_components<const global_transform>(en);
    }
    const auto &node = ctx.get_node(en);
    const auto has_parent_transform =
        !node.is_root()
        && !node.parent().entities().is_empty()
        && reg.has_components<global_transform>(node.parent().entities()[0]);
    auto [local, global] = reg.get_components<const transform, global_transform>(en);
    if(!has_parent_transform) {
        global->global = *local;
    } else {
        auto parent_global = reg.get_components<const global_transform>(node.parent().entities()[0]);
        global->global.set_matrix(parent_global->global.matrix() * local->matrix());
    }
    reg.remove_component<dirty_flag>(en);
    return *reg.get_components<const global_transform>(en);
}

} // namespace st
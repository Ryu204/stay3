module;

#include <cassert>
#include <functional>
#include <unordered_map>

export module stay3.node:tree_context;

import stay3.ecs;

import :node;
import :node_registry;

export namespace st {

class tree_context: public node_registry {
public:
    virtual ~tree_context() = default;

    [[nodiscard]] node::id_type register_node(node &node) override {
        node.on_entity_created().connect<&tree_context::add_entity_node_mapping>(*this);
        node.on_entity_destroyed().connect<&tree_context::remove_entity_node_mapping>(*this);
        return node_registry::register_node(node);
    }

    using node_registry::get_node;
    [[nodiscard]] node &get_node(entity en) {
        assert(m_entity_to_node.contains(en) && "Invalid entity");
        return m_entity_to_node.at(en);
    }

    [[nodiscard]] ecs_registry &ecs() {
        return m_ecs_reg;
    }

private:
    void add_entity_node_mapping(node &node, entity en) {
        m_entity_to_node.emplace(en, node);
    }
    void remove_entity_node_mapping(node &, entity en) {
        m_entity_to_node.erase(en);
    }

    ecs_registry m_ecs_reg;
    std::unordered_map<entity, std::reference_wrapper<node>, entity_hasher> m_entity_to_node;
};
} // namespace st
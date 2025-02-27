module;

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

export module stay3.node:tree_context;

import stay3.ecs;

import :node;

export namespace st {

class tree_context {
public:
    [[nodiscard]] node &get_node(entity en) {
        assert(m_entity_to_node.contains(en) && "Invalid entity");
        return m_entity_to_node.at(en);
    }

    [[nodiscard]] ecs_registry &ecs() {
        return m_ecs_reg;
    }

    [[nodiscard]] const node &get_node(const node::id_type &id) const {
        assert(m_id_to_node.contains(id) && "Unregistered id");
        return m_id_to_node.at(id);
    }

    [[nodiscard]] node &get_node(const node::id_type &id) {
        return const_cast<node &>(std::as_const(*this).get_node(id));
    }

    [[nodiscard]] node &get_root() const {
        assert(m_root_id.has_value() && "Root was not registered");
        return m_id_to_node.at(m_root_id.value());
    }

    [[nodiscard]] std::unique_ptr<node> create_root() {
        // We cannot use `std::make_unique` because the constructor is private
        auto result = std::unique_ptr<node>{new node{*this}};
        set_root(*result);
        return result;
    }

private:
    friend class node;

    void add_entity_node_mapping(node &node, entity en) {
        m_entity_to_node.emplace(en, node);
    }

    void remove_entity_node_mapping(node &, entity en) {
        m_entity_to_node.erase(en);
    }

    [[nodiscard]] node::id_type register_node(node &node) {
        node.on_entity_created().connect<&tree_context::add_entity_node_mapping>(*this);
        node.on_entity_destroyed().connect<&tree_context::remove_entity_node_mapping>(*this);
        const auto new_id = m_id_generator.create();
        m_id_to_node.emplace(new_id, node);
        return new_id;
    }

    void unregister_node(const node::id_type &id) {
        assert(m_id_to_node.contains(id) && "Node was not registered");
        m_id_to_node.erase(id);
    }

    void set_root(node &node) {
        assert(!m_root_id.has_value() && "Root was set");
        assert(m_id_to_node.contains(node.id()) && "Node was not registered");
        m_root_id = node.id();
    }

    std::optional<node::id_type> m_root_id;
    std::unordered_map<node::id_type, std::reference_wrapper<node>> m_id_to_node;
    id_generator<node::id_type> m_id_generator;

    ecs_registry m_ecs_reg;
    std::unordered_map<entity, std::reference_wrapper<node>, entity_hasher> m_entity_to_node;
};
} // namespace st
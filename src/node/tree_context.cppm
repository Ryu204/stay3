module;

#include <cassert>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

export module stay3.node:tree_context;

import stay3.core;
import stay3.ecs;

import :node;

export namespace st {

class tree_context {
public:
    struct node_reparented_args {
        node::id_type current;
        node::id_type old_parent;
        node::id_type new_parent;
    };
    decltype(auto) on_node_reparented() {
        return this->m_node_reparented_sink;
    }
    decltype(auto) on_entity_created() {
        return this->m_entity_created_sink;
    }
    decltype(auto) on_entity_destroyed() {
        return this->m_entity_destroyed_sink;
    }

    tree_context() {
        m_ecs_reg.on_entity_destroyed().connect<&tree_context::remove_entity_node_mapping>(*this);
    }
    ~tree_context() {
        m_root.reset();
    }
    tree_context(const tree_context &) = delete;
    tree_context &operator=(const tree_context &) = delete;
    tree_context(tree_context &&) noexcept = delete;
    tree_context &operator=(tree_context &&) noexcept = delete;

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

    [[nodiscard]] node &root() {
        if(!m_root) {
            // We cannot use `std::make_unique` because the constructor is private
            m_root = std::unique_ptr<node>{new node{*this}};
        }
        return *m_root;
    }

private:
    friend class node;

    void add_entity_node_mapping(node &node, entity en) {
        m_entity_to_node.emplace(en, node);
    }

    void remove_entity_node_mapping(entity en) {
        m_entity_to_node.erase(en);
    }

    [[nodiscard]] node::id_type register_node(node &node) {
        const auto new_id = m_id_generator.create();
        m_id_to_node.emplace(new_id, node);
        return new_id;
    }

    void unregister_node(const node::id_type &id) {
        assert(m_id_to_node.contains(id) && "Node was not registered");
        m_id_to_node.erase(id);
    }

    std::unique_ptr<node> m_root;
    std::unordered_map<node::id_type, std::reference_wrapper<node>> m_id_to_node;
    id_generator<node::id_type> m_id_generator;

    signal<void(node_reparented_args)> m_node_reparented;
    sink<decltype(m_node_reparented)> m_node_reparented_sink{m_node_reparented};

    ecs_registry m_ecs_reg;
    std::unordered_map<entity, std::reference_wrapper<node>, entity_hasher, entity_equal> m_entity_to_node;
    signal<void(node &, entity)> m_entity_created;
    sink<decltype(m_entity_created)> m_entity_created_sink{m_entity_created};
    signal<void(node &, entity)> m_entity_destroyed;
    sink<decltype(m_entity_destroyed)> m_entity_destroyed_sink{m_entity_destroyed};
};
} // namespace st
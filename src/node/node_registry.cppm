module;

#include <cassert>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <utility>

export module stay3.node:node_registry;

import stay3.system;
import :node;

export namespace st {

class node_registry {
public:
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

    void register_root(node &node) {
        assert(!m_root_id.has_value() && "Root was registered");
        assert(m_id_to_node.contains(node.id()) && "Node was not registered");
        m_root_id = node.id();
    }
    node::id_type register_node(node &node) {
        const auto new_id = m_id_generator.create();
        m_id_to_node.emplace(new_id, node);
        return new_id;
    }
    void unregister_node(const node::id_type &id) {
        assert(m_id_to_node.contains(id) && "Node was not registered");
        m_id_to_node.erase(id);
    }

private:
    std::optional<node::id_type> m_root_id;
    std::unordered_map<node::id_type, std::reference_wrapper<node>> m_id_to_node;
    id_generator<node::id_type> m_id_generator;
};
} // namespace st

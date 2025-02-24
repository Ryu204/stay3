module;

#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>

export module stay3.node:node_registry;

import :node;

export namespace st {

class node_registry {
public:
    [[nodiscard]] const node &get_node(const node::id_type &id) const;
    [[nodiscard]] node &get_node(const node::id_type &id);

    node::id_type register_node(node &node);
    void unregister_node(const node::id_type &id);

private:
    std::optional<node::id_type> m_root_id;
    std::unordered_map<node::id_type, std::reference_wrapper<node>> m_id_to_node;
};
} // namespace st

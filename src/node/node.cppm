module;

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

export module stay3.node:node;

import stay3.ecs;
import stay3.system;

export namespace st {

class tree_context;
class node {
public:
    using id_type = std::uint32_t;

    node(tree_context &context);
    ~node();
    node(node &&) noexcept = delete;
    node(const node &) = delete;
    node &operator=(node &&) noexcept = delete;
    node &operator=(const node &) = delete;

    node &add_child();
    [[nodiscard]] node &parent() const;
    void reparent(node &other);
    [[nodiscard]] bool is_ancestor_of(const node &other) const;
    [[nodiscard]] node &child(const id_type &id) const;
    void destroy_child(const id_type &id);

    [[nodiscard]] id_type id() const;

    [[nodiscard]] static std::unique_ptr<node> create_root(tree_context &context);

    [[nodiscard]] entities_holder &entities();
    [[nodiscard]] const entities_holder &entities() const;
    decltype(auto) on_entity_created() {
        return this->m_entity_created_sink;
    }
    decltype(auto) on_entity_destroyed() {
        return this->m_entity_destroyed_sink;
    }

private:
    void entity_created_handler(entity en);
    void entity_destroyed_handler(entity en);

    id_type m_id{};
    node *m_parent{};
    std::unordered_map<id_type, std::unique_ptr<node>> m_children;
    std::reference_wrapper<tree_context> m_tree_context;

    entities_holder m_entities;
    signal<void(node &, entity)> m_entity_created;
    sink<decltype(m_entity_created)> m_entity_created_sink{m_entity_created};
    signal<void(node &, entity)> m_entity_destroyed;
    sink<decltype(m_entity_destroyed)> m_entity_destroyed_sink{m_entity_destroyed};
};
} // namespace st

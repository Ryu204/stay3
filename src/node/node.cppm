module;

#include <functional>
#include <memory>
#include <unordered_map>

export module stay3.node:node;

export namespace st {

class node_registry;
class node {
public:
    using id_type = std::size_t;

    node(node_registry &registry);
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

    [[nodiscard]] static std::unique_ptr<node> create_root(node_registry &registry);

private:
    id_type m_id{};
    node *m_parent{};
    std::unordered_map<id_type, std::unique_ptr<node>> m_children;
    std::reference_wrapper<node_registry> m_registry;
};
} // namespace st

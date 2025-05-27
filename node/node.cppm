module;

#include <cstdint>
#include <functional>
#include <memory>
#include <unordered_map>

export module stay3.node:node;

import stay3.ecs;
import stay3.core;

export namespace st {

class tree_context;
class node {
public:
    using id_type = std::uint32_t;

    class iterator {
        using internal = std::unordered_map<id_type, std::unique_ptr<node>>::iterator;

    public:
        iterator(internal it)
            : m_it{it} {}

        node &operator*() {
            return *m_it->second;
        }
        iterator &operator++() {
            ++m_it;
            return *this;
        }
        bool operator==(const iterator &other) const {
            return m_it == other.m_it;
        }

    private:
        internal m_it;
    };

    class const_iterator {
        using internal = std::unordered_map<id_type, std::unique_ptr<node>>::const_iterator;

    public:
        const_iterator(internal it)
            : m_it{it} {}

        const node &operator*() const {
            return *m_it->second;
        }
        const_iterator &operator++() {
            ++m_it;
            return *this;
        }
        bool operator==(const const_iterator &other) const {
            return m_it == other.m_it;
        }

    private:
        internal m_it;
    };

    ~node();
    node(node &&) noexcept = delete;
    node(const node &) = delete;
    node &operator=(node &&) noexcept = delete;
    node &operator=(const node &) = delete;

    node &add_child();
    [[nodiscard]] node &parent() const;
    [[nodiscard]] bool is_root() const;
    void reparent(node &other);
    [[nodiscard]] bool is_ancestor_of(const node &other) const;
    [[nodiscard]] node &child(const id_type &id) const;
    void destroy_child(const id_type &id);

    /**
     * @brief Iterates over children nodes
     */
    const_iterator begin() const;
    const_iterator end() const;
    iterator begin();
    iterator end();
    /**
     * @brief Apply `function` recursively, using parent's result as child's parameter
     * @note Traversal is pre-order DFS. `ret`
     */
    template<typename func, typename... rets>
        requires(sizeof...(rets) == 0 || sizeof...(rets) == 1)
    void traverse(const func &function, const rets &...initials) {
        if constexpr(sizeof...(rets) == 1) {
            auto result = function(*this, initials...);
            for(auto &&[id, ptr]: m_children) {
                ptr->traverse(function, result);
            }
        } else {
            function(*this);
            for(auto &&[id, ptr]: m_children) {
                ptr->traverse(function);
            }
        }
    }

    [[nodiscard]] id_type id() const;

    [[nodiscard]] entities_holder &entities();
    [[nodiscard]] const entities_holder &entities() const;

private:
    friend class tree_context;

    node(tree_context &context);
    void entity_created_handler(entity en);
    void entity_destroyed_handler(entity en);

    id_type m_id{};
    node *m_parent{};
    std::unordered_map<id_type, std::unique_ptr<node>> m_children;
    std::reference_wrapper<tree_context> m_tree_context;

    entities_holder m_entities;
};
} // namespace st

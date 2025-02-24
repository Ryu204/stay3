module;

#include <cassert>
#include <memory>

module stay3.node;

import :node_registry;

namespace st {
node::node(node_registry &registry)
    : m_registry{registry} {
    m_id = m_registry.get().register_node(*this);
}

node::~node() {
    m_registry.get().unregister_node(m_id);
}

node &node::add_child() {
    auto child_ptr = std::make_unique<node>(m_registry);
    auto &result = *child_ptr;
    child_ptr->m_parent = this;
    const auto id = child_ptr->m_id;
    m_children.emplace(id, std::move(child_ptr));
    return result;
}

node &node::parent() const {
    assert(m_parent != nullptr && "Node does not have parent");
    return *m_parent;
}

void node::reparent(node &other) {
    assert(this != &other && "Parent to self");
    assert(!is_ancestor_of(other) && "Cyclic relationship");
    assert(m_parent != nullptr && "Node does not have parent");
    assert(std::addressof(m_registry.get()) == std::addressof(other.m_registry.get())
           && "Nodes from 2 different trees");
    // Remove from old parent
    auto this_unique_ptr = std::move(m_parent->m_children[m_id]);
    m_parent->m_children.erase(m_id);
    // Append to new parent
    m_parent = &other;
    other.m_children.emplace(m_id, std::move(this_unique_ptr));
}

bool node::is_ancestor_of(const node &other) const {
    for(auto *i = other.m_parent; i != nullptr; i = i->m_parent) {
        if(this == i) {
            return true;
        }
    }
    return false;
}

node &node::child(const id_type &id) const {
    assert(m_children.contains(id) && "Not a child node");
    return *m_children.at(id);
}

void node::destroy_child(const id_type &id) {
    assert(m_children.contains(id) && "Not a child node");
    m_children.erase(id);
}

node::id_type node::id() const {
    return m_id;
}

std::unique_ptr<node> node::create_root(node_registry &registry) {
    auto result = std::make_unique<node>(registry);
    registry.register_root(*result);
    return result;
}

} // namespace st

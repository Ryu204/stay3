module;

#include <cassert>
#include <memory>
#include <utility>

module stay3.node;

import stay3.ecs;
import :tree_context;

namespace st {

node::node(tree_context &context)
    : m_tree_context{context}, m_entities{context.ecs()} {
    m_id = m_tree_context.get().register_node(*this);
    m_entities.on_create().connect<&node::entity_created_handler>(*this);
    m_entities.on_destroy().connect<&node::entity_destroyed_handler>(*this);
}

node::~node() {
    m_tree_context.get().unregister_node(m_id);
}

node &node::add_child() {
    // Constructor is private so no `std::make_unique`
    auto child_ptr = std::unique_ptr<node>{new node{m_tree_context}};
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

bool node::is_root() const {
    return m_parent == nullptr;
}

void node::reparent(node &other) {
    assert(this != &other && "Parent to self");
    assert(!is_ancestor_of(other) && "Cyclic relationship");
    assert(m_parent != nullptr && "Node does not have parent");
    assert(std::addressof(m_tree_context.get()) == std::addressof(other.m_tree_context.get())
           && "Nodes from 2 different trees");
    // Remove from old parent
    auto this_unique_ptr = std::move(m_parent->m_children[m_id]);
    m_parent->m_children.erase(m_id);
    // Append to new parent
    auto *old_parent = m_parent;
    m_parent = &other;
    other.m_children.emplace(m_id, std::move(this_unique_ptr));
    m_tree_context.get().m_node_reparented.publish({.current = m_id, .old_parent = old_parent->m_id, .new_parent = m_parent->m_id});
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

node::const_iterator node::begin() const {
    return m_children.cbegin();
}

node::const_iterator node::end() const {
    return m_children.cend();
}

node::iterator node::begin() {
    return m_children.begin();
}

node::iterator node::end() {
    return m_children.end();
}

node::id_type node::id() const {
    return m_id;
}

const entities_holder &node::entities() const {
    return m_entities;
}

entities_holder &node::entities() {
    return const_cast<entities_holder &>(std::as_const(*this).entities());
}

void node::entity_created_handler(entity en) {
    m_tree_context.get().add_entity_node_mapping(*this, en);
    m_tree_context.get().m_entity_created.publish(*this, en);
}

void node::entity_destroyed_handler(entity en) {
    m_tree_context.get().m_entity_destroyed.publish(*this, en);
}

} // namespace st

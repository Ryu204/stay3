module;

#include <cassert>

#include <entt/entt.hpp>

export module stay3.ecs:entity;

export namespace st {

class ecs_registry;
class entity_hasher;
class entity_equal;

/**
 * @brief Transparent entity type, should not be inspected
 */
class entity {
public:
    entity(entt::entity en)
        : m_raw{en} {}
    entity()
        : m_raw{entt::null} {}
    operator entt::entity() const {
        return m_raw;
    }
    [[nodiscard]] bool is_null() const {
        return m_raw == entt::null;
    }
    [[nodiscard]] std::size_t numeric() const {
        assert(!is_null() && "Null entity access");
        return static_cast<std::size_t>(m_raw);
    }
    static entity from_numeric(std::size_t val) {
        return {static_cast<entt::entity>(val)};
    }

private:
    friend class entity_hasher;
    friend class entity_equal;
    entt::entity m_raw;
};

struct entity_hasher: std::hash<entt::entity> {
    std::size_t operator()(const entity &en) const noexcept {
        return std::hash<entt::entity>::operator()(en.m_raw);
    }
};

struct entity_equal {
    bool operator()(const entity &lhs, const entity &rhs) const noexcept {
        return lhs.m_raw == rhs.m_raw;
    }
};
} // namespace st
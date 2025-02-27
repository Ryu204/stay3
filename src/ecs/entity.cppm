module;

#include <entt/entt.hpp>

export module stay3.ecs:entity;

export namespace st {

class ecs_registry;

/**
 * @brief Transparent entity type, should not be inspected
 */
struct entity {
    entity(entt::entity e)
        : raw{e} {}
    entity()
        : raw{entt::null} {}
    operator entt::entity() const {
        return raw;
    }
    entt::entity raw;
};

struct entity_hasher: std::hash<entt::entity> {
    std::size_t operator()(const entity &en) const noexcept {
        return std::hash<entt::entity>::operator()(en.raw);
    }
};
} // namespace st
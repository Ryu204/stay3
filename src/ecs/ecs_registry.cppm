module;

#include <concepts>
#include <type_traits>
#include <utility>
#include <entt/entt.hpp>

export module stay3.ecs:ecs_registry;

import :entity;

export namespace st {

template<typename type>
concept component = !std::is_same_v<entity, std::decay_t<type>>;

class ecs_registry {
public:
    [[nodiscard]] entity create_entity() {
        return m_registry.create();
    }

    void destroy_entity(entity en) {
        m_registry.destroy(en);
    }

    [[nodiscard]] bool contains_entity(entity en) const {
        return m_registry.valid(en);
    }

    template<component comp, typename... arguments>
    void add_component(entity en, arguments &&...args) {
        m_registry.emplace<comp>(en, std::forward<arguments>(args)...);
    }

    template<component comp>
    void remove_component(entity en) {
        m_registry.remove<comp>(en);
    }

    template<component comp, typename func>
        requires std::invocable<func, comp &>
    void patch_component(entity en, func &&patcher) {
        m_registry.patch<comp>(en, std::forward<func>(patcher));
    }

    template<component comp, typename... arguments>
    void replace_component(entity en, arguments &&...args) {
        m_registry.replace<comp>(en, std::forward<arguments>(args)...);
    }

    template<component... comps>
    bool has_components(entity en) {
        return m_registry.all_of<comps...>(en);
    }

    template<component comp>
    comp &get_component(entity en) {
        return m_registry.get<comp>(en);
    }

private:
    entt::registry m_registry;
};
} // namespace st
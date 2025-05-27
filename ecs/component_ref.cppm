module;

#include <cassert>

export module stay3.ecs:component_ref;

import :entity;
import :ecs_registry;
import :component;

export namespace st {
/**
 * @brief Alternative to reference the component from its entity
 */
template<component comps>
class component_ref {
    static_assert(!is_mut_v<comps> && "Use `get_mut` instead of mut template parameter");

public:
    component_ref(entity en = {})
        : m_entity{en} {}
    component_ref &operator=(entity en) {
        m_entity = en;
        return *this;
    }
    /**
     * @return Read access proxy to the component
     */
    [[nodiscard]] auto get(ecs_registry &reg) const {
        assert(!m_entity.is_null() && "Null component reference");
        return reg.get<comps>(m_entity);
    }
    /**
     * @return Write access proxy to the component
     */
    [[nodiscard]] auto get_mut(ecs_registry &reg) const {
        assert(!m_entity.is_null() && "Null component reference");
        return reg.get<add_mut_t<comps>>(m_entity);
    }
    [[nodiscard]] entity entity() const {
        return m_entity;
    }
    [[nodiscard]] bool is_null() const {
        return m_entity.is_null();
    }
    [[nodiscard]] bool operator==(const component_ref &other) const {
        return m_entity == other.m_entity;
    }
    [[nodiscard]] bool operator!=(const component_ref &other) const {
        return m_entity != other.m_entity;
    }

private:
    struct entity m_entity;
};
} // namespace st
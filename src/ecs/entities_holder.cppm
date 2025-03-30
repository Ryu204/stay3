module;

#include <cassert>
#include <concepts>
#include <cstddef>
#include <entt/entt.hpp>

export module stay3.ecs:entities_holder;

import stay3.core;

import :entity;
import :ecs_registry;

export namespace st {
/**
 * @brief Owns a set of entities
 */
class entities_holder {
public:
    class const_iterator {
        using internal = std::vector<entity>::const_iterator;

    public:
        using difference_type = std::ptrdiff_t;
        const_iterator(internal it)
            : m_it{it} {};
        const entity &operator*() const {
            return *m_it;
        }
        const_iterator &operator++() {
            ++m_it;
            return *this;
        }
        bool operator==(const const_iterator &other) const {
            return m_it == other.m_it;
        }
        const_iterator operator+(difference_type n) const {
            return m_it + n;
        }

    private:
        internal m_it;
    };

    entities_holder(ecs_registry &reg)
        : m_registry{reg} {};
    ~entities_holder() {
        clear();
    }
    entities_holder(entities_holder &&) noexcept = delete;
    entities_holder(const entities_holder &) = delete;
    entities_holder &operator=(entities_holder &&) noexcept = delete;
    entities_holder &operator=(const entities_holder &) noexcept = delete;

    /**
     * @brief Creates new entity
     */
    entity create() {
        const auto result = m_registry.get().create();
        m_entities.emplace_back(result);
        m_entity_created.publish(result);
        return result;
    }

    /**
     * @brief Erases entity by index
     * @brief Entity at index 0 must be destroyed last
     */
    void destroy(std::ptrdiff_t index) {
        assert(m_entities.size() > index && "Out of range");
        assert(
            (index > 0 || m_entities.size() == 1)
            && "Entity at index 0 must be destroyed last");
        const auto destroyed_entity = m_entities[index];
        m_on_entity_destroy.publish(destroyed_entity);
        m_registry.get().destroy(destroyed_entity);
        m_entities.erase(m_entities.begin() + index);
    }

    /**
     * @brief Erases entity by value
     * @note Entity at index 0 must be destroyed last
     */
    void destroy(entity en) {
        auto it = std::ranges::find(m_entities, en);
        assert(it != m_entities.end() && "Entity not found");

        std::ptrdiff_t index = std::distance(m_entities.begin(), it);
        destroy(index);
    }

    [[nodiscard]] std::size_t size() const {
        return m_entities.size();
    }

    [[nodiscard]] bool is_empty() const {
        return m_entities.empty();
    }

    [[nodiscard]] const entity &operator[](std::size_t index) const {
        assert(index < m_entities.size() && "Out of range");
        return m_entities[index];
    }

    [[nodiscard]] const_iterator begin() const {
        return m_entities.cbegin();
    }

    [[nodiscard]] const_iterator end() const {
        return m_entities.cend();
    }

    /**
     * @note Do not destroy entity here
     */
    template<typename func>
        requires std::invocable<func, ecs_registry &, entity>
    void each(const func &function) {
        for(auto en: m_entities) {
            function(m_registry, en);
        }
    }

    void clear() {
        while(!m_entities.empty()) {
            destroy(static_cast<std::ptrdiff_t>(m_entities.size()) - 1);
        }
    }

    /**
     * @brief Signal after entity creation
     */
    decltype(auto) on_create() {
        return this->m_entity_created_sink;
    }

    /**
     * @brief Signal before entity destruction
     */
    decltype(auto) on_destroy() {
        return this->m_on_entity_destroy_sink;
    }

private:
    std::reference_wrapper<ecs_registry> m_registry;
    std::vector<entity> m_entities;

    signal<void(entity)> m_entity_created;
    sink<decltype(m_entity_created)> m_entity_created_sink{m_entity_created};
    signal<void(entity)> m_on_entity_destroy;
    sink<decltype(m_on_entity_destroy)> m_on_entity_destroy_sink{m_on_entity_destroy};
};

} // namespace st
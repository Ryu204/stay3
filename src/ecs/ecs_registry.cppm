module;

#include <concepts>
#include <cstdint>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <entt/entt.hpp>

export module stay3.ecs:ecs_registry;

import stay3.core;
import :entity;

namespace st {

template<typename type>
concept component = !std::is_same_v<entity, std::decay_t<type>>;

export enum class comp_event: std::uint8_t {
    construct,
    destroy,
    update,
};

template<component comp>
class write_access_proxy {
public:
    write_access_proxy(ecs_registry &reg, entity en)
        : m_registry{&reg}, m_entity{en}, m_component{&reg.m_registry.get<comp>(en)} {}
    ~write_access_proxy() {
        publish_update_event();
    }
    write_access_proxy(const write_access_proxy &) = delete;
    write_access_proxy(write_access_proxy &&other) noexcept {
        if(this == &other) {
            return;
        }
        publish_update_event();
        m_registry = other.m_registry;
        m_entity = other.m_entity;
        m_component = other.m_component;
        other.m_registry = nullptr;
    }
    write_access_proxy &operator=(const write_access_proxy &) = delete;
    write_access_proxy &operator=(write_access_proxy &&) noexcept = delete;

    comp *operator->() {
        return m_component;
    }

private:
    void publish_update_event() noexcept {
        try {
            if(m_registry != nullptr) {
                m_registry->publish_event<comp_event::update, comp>(m_registry->m_registry, m_entity);
            }
        } catch(std::exception &e) {
            assert(false && "Unknown exception");
        } catch(...) {
            assert(false && "Unknown error");
        }
    }

    ecs_registry *m_registry;
    entity m_entity;
    comp *m_component;
};
template<component comp>
using get_return_t = std::conditional_t<
    std::is_const_v<std::remove_reference_t<comp>>,
    std::add_lvalue_reference_t<comp>,
    write_access_proxy<comp>>;

export class ecs_registry {
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

    /**
     * @note Getting non const reference will publish an update event
     */
    template<component comp>
    get_return_t<comp> get_component(entity en) {
        if constexpr(std::is_const_v<std::remove_reference_t<comp>>) {
            return m_registry.get<comp>(en);
        } else {
            return write_access_proxy<comp>{*this, en};
        }
    }

    /**
     * @example
     * ```
     * for (auto &&[en, c1, c2] : registry.each<comp1, comp2>()) {
     *   // Access components here...
     *}
     *```
     */
    template<component... comps>
    auto each() {
        return m_registry.view<comps...>().each();
    }

    template<component comp, typename pred>
        requires std::invocable<pred, entity, entity>
    void sort(pred &&func) {
        m_registry.sort<comp>(std::forward<pred>(func));
    }

    template<comp_event ev, component comp>
    decltype(auto) on() {
        const auto key = event_key<ev, comp>;
        if(!m_component_signals.contains(key)) {
            create_event<ev, comp>();
        }
        return m_component_signals[key].sk;
    }

private:
    template<comp_event ev, component comp>
    void create_event() {
        auto entt_sink = m_registry.on_construct<comp>();
        if constexpr(ev == comp_event::destroy) {
            entt_sink = m_registry.on_destroy<comp>();
        }
        if constexpr(ev == comp_event::update) {
            entt_sink = m_registry.on_update<comp>();
        }

        m_component_signals.try_emplace(event_key<ev, comp>);
        entt_sink.template connect<&ecs_registry::publish_event<ev, comp>>(*this);
    }

    /**
     * @brief Helper to publish an event if it was observed by `on`
     */
    template<comp_event ev, component comp>
    void publish_event(entt::registry &, entity en) {
        auto itr = m_component_signals.find(event_key<ev, comp>);
        if(itr == m_component_signals.end())
            return;
        itr->second.sig.publish(*this, en);
    }

    using component_event = std::pair<comp_event, std::type_index>;
    using signal_signature = void(ecs_registry &, entity);
    struct signal_pair {
        signal<signal_signature> sig;
        sink<decltype(sig)> sk{sig};
    };
    struct hasher {
        std::size_t operator()(const component_event &ev) const noexcept {
            std::size_t h1 = std::hash<comp_event>{}(ev.first);
            std::size_t h2 = std::hash<std::type_index>{}(ev.second);
            // Combine the hashes using a common bit-mixing method.
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    template<comp_event ev, component comp>
    static inline const component_event event_key{ev, typeid(std::decay_t<comp>)};
    entt::registry m_registry;
    std::unordered_map<component_event, signal_pair, hasher> m_component_signals;
};
} // namespace st

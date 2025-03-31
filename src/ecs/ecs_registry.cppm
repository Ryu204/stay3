module;

#include <concepts>
#include <iterator>
#include <ranges>
#include <type_traits>
#include <typeindex>
#include <utility>
#include <entt/entt.hpp>

export module stay3.ecs:ecs_registry;

import stay3.core;
import :entity;
import :component;

export namespace st {

/**
 * @brief tag class for mutable reference to component
 */
template<typename comp>
struct mut {};
template<typename comp>
struct is_mut {
    static constexpr auto value = false;
};
template<component comp>
struct is_mut<mut<comp>> {
    static constexpr auto value = true;
};
template<typename type>
constexpr auto is_mut_v = is_mut<type>::value;
template<typename t>
struct remove_mut {
    using type = t;
};
template<component comp>
struct remove_mut<mut<comp>> {
    using type = comp;
};
template<typename t>
using remove_mut_t = remove_mut<t>::type;

template<typename... ts>
struct exclude_t {};
template<typename... ts>
constexpr exclude_t<ts...> exclude{};

class ecs_registry {
    template<component ecomp>
    class empty_proxy {
        static_assert(!is_mut_v<ecomp>);
        static_assert(std::is_empty_v<ecomp>);
    };

    template<component comp>
    class read_access_proxy {
        static_assert(!is_mut_v<comp>);
        static_assert(std::is_const_v<std::remove_reference_t<comp>>);

    public:
        read_access_proxy(comp &data)
            : m_data{data} {}

        comp *operator->() const {
            return std::addressof(m_data.get());
        }
        comp &operator*() const {
            return m_data.get();
        }

    private:
        std::reference_wrapper<comp> m_data;
    };

    template<typename comp>
    class write_access_proxy {};
    template<component comp>
    class write_access_proxy<mut<comp>> {
        static_assert(!std::is_const_v<std::remove_reference_t<comp>>);

    public:
        write_access_proxy(ecs_registry &reg, entity en)
            : m_registry{&reg}, m_entity{en}, m_component{&reg.m_registry.get<comp>(en)} {}
        ~write_access_proxy() {
            publish_update_event();
        }
        write_access_proxy(const write_access_proxy &) = delete;
        write_access_proxy(write_access_proxy &&other) noexcept
            : m_registry{other.m_registry}, m_entity{other.m_entity}, m_component{other.m_component} {
            other.m_registry = nullptr;
        }
        write_access_proxy &operator=(const write_access_proxy &) = delete;
        write_access_proxy &operator=(write_access_proxy &&) noexcept = delete;

        comp *operator->() {
            assert(m_registry != nullptr && "Stale proxy");
            return m_component;
        }
        comp &operator*() {
            assert(m_registry != nullptr && "Stale proxy");
            return *m_component;
        }

    private:
        void publish_update_event() noexcept {
            try {
                if(m_registry != nullptr) {
                    m_registry->publish_event<comp_event::update, comp>(m_registry->m_registry, m_entity);
                }
            } catch(std::exception &e) {
                log::error("Error at component event listener: ", e.what());
                assert(false);
            } catch(...) {
                assert(false && "Unknown error");
            }
        }

        ecs_registry *m_registry;
        entity m_entity;
        comp *m_component;
    };

    template<component comp>
    using proxy = std::conditional_t<is_mut_v<comp>,
                                     write_access_proxy<comp>,
                                     std::conditional_t<std::is_empty_v<comp>,
                                                        empty_proxy<comp>,
                                                        read_access_proxy<std::add_const_t<comp>>>>;

    template<component comp>
    proxy<comp> make_proxy(entity en) {
        if constexpr(is_mut_v<comp>) {
            return write_access_proxy<comp>{*this, en};
        } else if constexpr(std::is_empty_v<comp>) {
            return {};
        } else {
            return m_registry.get<std::decay_t<comp>>(en);
        }
    }

    template<typename entt_it, component... comps>
    class entity_components_view_iterator {
    public:
        using difference_type = entt_it::difference_type;
        using value_type = std::tuple<entity, proxy<comps>...>;
        entity_components_view_iterator(entt_it it = {}, ecs_registry *reg = nullptr)
            : m_it{it}, m_registry{reg} {}
        value_type operator*() const {
            assert(m_registry != nullptr);
            entity en{*m_it};
            return {en, m_registry->make_proxy<comps>(en)...};
        }
        entity_components_view_iterator &operator++() {
            ++m_it;
            return *this;
        }
        bool operator!=(const entity_components_view_iterator &other) const {
            return m_it != other.m_it;
        }
        bool operator==(const entity_components_view_iterator &other) const {
            return m_it == other.m_it;
        }
        void operator++(int) {
            ++*this;
        }

    private:
        entt_it m_it;
        ecs_registry *m_registry;
    };

    template<typename entt_view, component... comps>
    class entity_components_view {
    public:
        using entt_it = std::decay_t<decltype(std::declval<entt_view>().begin())>;
        using iterator = entity_components_view_iterator<entt_it, comps...>;
        static_assert(std::input_iterator<iterator>);
        static_assert(std::semiregular<iterator>);
        static_assert(std::sentinel_for<iterator, iterator>);

        entity_components_view(entt_view view, ecs_registry &reg)
            : m_view{view}, m_registry{reg} {}
        iterator begin() {
            return {m_view.begin(), &m_registry.get()};
        }
        iterator end() {
            return {m_view.end(), &m_registry.get()};
        }

    private:
        entt_view m_view;
        std::reference_wrapper<ecs_registry> m_registry;
    };

    template<typename entt_it, component... comps>
    class entity_view_iterator {
    public:
        using difference_type = entt_it::difference_type;
        using value_type = entity;
        entity_view_iterator(entt_it it = {})
            : m_it{it} {}
        value_type operator*() const {
            return *m_it;
        }
        entity_view_iterator &operator++() {
            ++m_it;
            return *this;
        }
        bool operator!=(const entity_view_iterator &other) const {
            return m_it != other.m_it;
        }
        bool operator==(const entity_view_iterator &other) const {
            return m_it == other.m_it;
        }
        void operator++(int) {
            ++*this;
        }

    private:
        entt_it m_it;
    };

    template<typename entt_view, component... comps>
    class entity_view {
    public:
        using entt_it = std::decay_t<decltype(std::declval<entt_view>().begin())>;
        using iterator = entity_view_iterator<entt_it, comps...>;
        static_assert(std::input_iterator<iterator>);
        static_assert(std::semiregular<iterator>);
        static_assert(std::sentinel_for<iterator, iterator>);

        entity_view(entt_view view)
            : m_view{view} {}
        iterator begin() {
            return {m_view.begin()};
        }
        iterator end() {
            return {m_view.end()};
        }

    private:
        entt_view m_view;
    };

public:
    ecs_registry() {
        m_registry.on_destroy<entt::entity>().connect<&ecs_registry::entity_destroyed_handler>(*this);
    }

    [[nodiscard]] entity create() {
        return m_registry.create();
    }

    void destroy(entity en) {
        m_registry.destroy(en);
    }

    void destroy_if_exist(entity en) {
        if(m_registry.valid(en)) {
            m_registry.destroy(en);
        }
    }

    [[nodiscard]] bool contains(entity en) const {
        return m_registry.valid(en);
    }

    /**
     * @return A read or write proxy to the created component
     */
    template<component comp, typename... arguments>
    proxy<comp> emplace(entity en, arguments &&...args) {
        m_registry.emplace<std::decay_t<remove_mut_t<comp>>>(en, std::forward<arguments>(args)...);
        return get<comp>(en);
    }

    template<component comp, typename... arguments>
    proxy<comp> emplace_or_replace(entity en, arguments &&...args) {
        m_registry.emplace_or_replace<std::decay_t<remove_mut_t<comp>>>(en, std::forward<arguments>(args)...);
        return get<comp>(en);
    }

    template<component comp, typename... arguments>
    proxy<comp> emplace_if_not_exist(entity en, arguments &&...args) {
        using raw_comp = std::decay_t<remove_mut_t<comp>>;
        if(!m_registry.all_of<raw_comp>(en)) {
            m_registry.emplace<raw_comp>(en, std::forward<arguments>(args)...);
        }
        return get<comp>(en);
    }

    template<component... comps>
    void destroy(entity en) {
        m_registry.erase<comps...>(en);
    }

    template<component... comps>
    void destroy_if_exist(entity en) {
        m_registry.remove<comps...>(en);
    }

    template<component comp>
    void destroy_all() {
        m_registry.clear<comp>();
    }

    template<component comp, typename func>
        requires std::invocable<func, comp &>
    void patch(entity en, func &&patcher) {
        m_registry.patch<comp>(en, std::forward<func>(patcher));
    }

    template<component comp, typename... arguments>
    void replace(entity en, arguments &&...args) {
        m_registry.replace<comp>(en, std::forward<arguments>(args)...);
    }

    template<component... comps>
        requires(sizeof...(comps) > 0)
    [[nodiscard]] bool contains(entity en) const {
        return m_registry.all_of<comps...>(en);
    }

    /**
     * @note Will publish an update event of a `comp` if it is marked `mut<comp>`
     * and the returned result goes out of scope
     */
    template<component... comps>
        requires(sizeof...(comps) > 0)
    auto get(entity en) {
        if constexpr(sizeof...(comps) == 1) {
            return make_proxy<comps...>(en);
        } else {
            return std::make_tuple(make_proxy<comps>(en)...);
        }
    }

    /**
     * @example
     * ```
     * for (auto &&[en, c1, c2] : registry.each<comp1, comp2>()) {
     *     // Access components here...
     * }
     * ```
     */
    template<component... comps, component... exclude_comps>
    auto each(exclude_t<exclude_comps...> = {}) {
        using entt_view = std::decay_t<decltype(std::declval<entt::registry>()
                                                    .view<remove_mut_t<comps>...>(std::declval<entt::exclude_t<exclude_comps...>>()))>;
        using result = entity_components_view<entt_view, comps...>;
        static_assert(std::ranges::range<result>);
        return result{m_registry.view<remove_mut_t<comps>...>(entt::exclude<exclude_comps...>), *this};
    }

    template<component... comps, component... exclude_comps>
    auto view(exclude_t<exclude_comps...> = {}) {
        using entt_view = std::decay_t<decltype(std::declval<entt::registry>()
                                                    .view<remove_mut_t<comps>...>(std::declval<entt::exclude_t<exclude_comps...>>()))>;
        using result = entity_view<entt_view, comps...>;
        static_assert(std::ranges::range<result>);
        return result{m_registry.view<remove_mut_t<comps>...>(entt::exclude<exclude_comps...>)};
    }

    template<component comp, typename pred>
        requires is_sort_predicate<pred, comp>
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

    decltype(auto) on_entity_destroyed() {
        return this->m_entity_destroyed_sink;
    }

    template<typename context, typename... args>
    std::add_lvalue_reference_t<context> add_context(args &&...arguments) {
        return m_registry.ctx().emplace<context>(std::forward<args>(arguments)...);
    }

    template<typename context>
    std::add_lvalue_reference_t<context> get_context() {
        return m_registry.ctx().get<context>();
    }

private:
    template<comp_event ev, component comp>
    void create_event() {
        using decayed = std::decay_t<comp>;
        auto entt_sink = m_registry.on_construct<decayed>();
        if constexpr(ev == comp_event::destroy) {
            entt_sink = m_registry.on_destroy<decayed>();
        }
        if constexpr(ev == comp_event::update) {
            entt_sink = m_registry.on_update<decayed>();
        }

        m_component_signals.try_emplace(event_key<ev, comp>);
        entt_sink.template connect<&ecs_registry::publish_event<ev, comp>>(*this);
    }

    /**
     * @brief Helper to publish an event if it was observed by `on`
     */
    template<comp_event ev, component comp>
    void publish_event(entt::registry &, entity en) {
        if(!m_component_signals.contains(event_key<ev, comp>)) {
            return;
        }
        m_component_signals[event_key<ev, comp>].sig.publish(*this, en);
    }

    struct component_event {
        comp_event first;
        std::type_index second;
    };
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
            return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2)); // NOLINT
        }
    };
    struct equal {
        bool operator()(const component_event &lhs, const component_event &rhs) const {
            return lhs.first == rhs.first && ((lhs.second <=> rhs.second) == 0);
        }
    };

    void entity_destroyed_handler(entt::registry &, entt::entity en) {
        m_entity_destroyed.publish(en);
    }

    template<comp_event ev, component comp>
    static inline const component_event event_key{.first = ev, .second = typeid(std::decay_t<comp>)};
    entt::registry m_registry;
    std::unordered_map<component_event, signal_pair, hasher, equal> m_component_signals;
    signal<void(entity)> m_entity_destroyed;
    sink<decltype(m_entity_destroyed)> m_entity_destroyed_sink{m_entity_destroyed};
};
} // namespace st

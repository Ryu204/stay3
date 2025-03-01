module;

#include <cassert>
#include <functional>
#include <memory>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <variant>
#include <vector>

export module stay3.ecs:system_manager;

import stay3.system;
import :system_data;
import :system_wrapper;

export namespace st {

/**
 * @brief Manage systems registration and call their methods
 */
template<typename context>
class system_manager {
    struct system_entry_per_type {
        std::reference_wrapper<system_wrapper<context>> system;
        sys_custom_priority priority;
        bool operator<(const system_entry_per_type &other) const {
            constexpr auto has_higher_priority = [](const auto &higher, const auto &lower) {
                return higher.priority > lower.priority;
            };
            return has_higher_priority(*this, other);
        }
    };

    using wrapper = system_wrapper<context>;
    class proxy {
    public:
        using priority = std::variant<sys_priority, sys_custom_priority>;
        using types = std::underlying_type_t<sys_type>;

        proxy(wrapper &system, system_manager &manager)
            : m_system{system}, m_manager{manager} {}

        template<sys_type type>
        proxy &run_as(priority order = sys_priority::medium) {
            const auto order_as_num = std::visit(
                [](auto &&value) { return static_cast<sys_custom_priority>(value); },
                order);

            assert((static_cast<types>(type) & m_registered_types) == 0 && "System type registered twice");
            assert(m_system.get().template is_type<type>() && "System does not satisfy type");
            m_registered_types |= static_cast<types>(type);

            m_manager.get().m_systems_by_type[type].insert({.system = m_system, .priority = order_as_num});

            return *this;
        }

    private:
        types m_registered_types{};
        std::reference_wrapper<wrapper> m_system;
        std::reference_wrapper<system_manager<context>> m_manager;
    };

public:
    template<typename sys, typename... sys_ctor_args>
    proxy add(sys_ctor_args &&...args) {
        auto &sys_ptr = m_systems.emplace_back(std::make_unique<wrapper>(std::in_place_type<sys>, std::forward<sys_ctor_args>(args)...));
        return {*sys_ptr, *this};
    }

    void update(seconds delta, context &ctx) {
        apply_all<sys_type::update>(delta, ctx);
    }

    void start(context &ctx) {
        apply_all<sys_type::start>(ctx);
    }

    void cleanup(context &ctx) {
        apply_all<sys_type::cleanup>(ctx);
    }

    void render(context &ctx) {
        apply_all<sys_type::render>(ctx);
    }

private:
    template<sys_type type, typename... args>
    void apply_all(args &&...arguments) {
        for(const auto &entry: m_systems_by_type[type]) {
            entry.system.get().template call_method<type>(std::forward<args>(arguments)...);
        }
    }
    friend class proxy;
    std::unordered_map<sys_type, std::set<system_entry_per_type>> m_systems_by_type;
    std::vector<std::unique_ptr<wrapper>> m_systems;
};
} // namespace st
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

import stay3.core;
import stay3.input;
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
        sys_priority_t priority;
        bool operator<(const system_entry_per_type &other) const {
            if(priority == other.priority) {
                return std::addressof(system.get()) < std::addressof(other.system.get());
            }
            constexpr auto has_higher_priority = [](const auto &higher, const auto &lower) {
                return higher.priority > lower.priority;
            };
            return has_higher_priority(*this, other);
        }
    };

    using wrapper = system_wrapper<context>;
    /**
     * @brief Register system roles to manager
     */
    class base_proxy {
    public:
        using priority = std::variant<sys_priority, sys_priority_t>;
        using types = std::underlying_type_t<sys_type>;

        base_proxy(wrapper &system, system_manager &manager)
            : m_system{system}, m_manager{manager} {}

    protected:
        template<sys_type type>
        void run_as(priority order) {
            const auto order_as_num = std::visit(
                [](auto &&value) { return static_cast<sys_priority_t>(value); },
                order);

            assert((static_cast<types>(type) & m_registered_types) == 0 && "System type registered twice");
            m_registered_types |= static_cast<types>(type);

            m_manager.get().m_systems_by_type[type].insert({.system = m_system, .priority = order_as_num});
        }

    private:
        types m_registered_types{};
        std::reference_wrapper<wrapper> m_system;
        std::reference_wrapper<system_manager<context>> m_manager;
    };
    /**
     * @brief Provides compile time system roles check
     */
    template<typename sys>
    struct proxy: base_proxy {
        using base_proxy::base_proxy;

        template<sys_type type>
        proxy &run_as(base_proxy::priority order = sys_priority::medium)
            requires is_system_type<type, sys, context>
        {
            base_proxy::template run_as<type>(order);
            return *this;
        }
    };

public:
    template<typename sys, typename... sys_ctor_args>
    proxy<sys> add(sys_ctor_args &&...args) {
        auto &sys_ptr = m_systems.emplace_back(std::make_unique<wrapper>(std::in_place_type<sys>, std::forward<sys_ctor_args>(args)...));
        return {*sys_ptr, *this};
    }

    sys_run_result update(seconds delta, context &ctx) {
        return apply_all<sys_type::update>(delta, ctx);
    }

    sys_run_result start(context &ctx) {
        return apply_all<sys_type::start>(ctx);
    }

    sys_run_result cleanup(context &ctx) {
        return apply_all<sys_type::cleanup>(ctx);
    }

    sys_run_result render(context &ctx) {
        return apply_all<sys_type::render>(ctx);
    }

    sys_run_result input(const event &ev, context &ctx) {
        return apply_all<sys_type::input>(ev, ctx);
    }

    sys_run_result post_update(seconds delta, context &ctx) {
        return apply_all<sys_type::post_update>(delta, ctx);
    }

private:
    template<sys_type type, typename... args>
    sys_run_result apply_all(args &&...arguments) {
        auto res = sys_run_result::noop;
        for(const auto &entry: m_systems_by_type[type]) {
            auto cur = entry.system.get().template call_method<type>(std::forward<args>(arguments)...);
            switch(cur) {
            case sys_run_result::noop:
                break;
            case sys_run_result::exit:
                return sys_run_result::exit;
            default:
                // Potentially more control result types
                break;
            }
        }
        return res;
    }

    std::unordered_map<sys_type, std::set<system_entry_per_type>> m_systems_by_type;
    std::vector<std::unique_ptr<wrapper>> m_systems;
};
} // namespace st
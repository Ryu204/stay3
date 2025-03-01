module;

#include <any>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

export module stay3.ecs:system_wrapper;

import stay3.system;
import :system_data;

export namespace st {
/**
 * @brief Wrapper around an arbitrary object as a system with callable methods
 */
template<typename context>
class system_wrapper {
public:
    template<typename sys, typename... args>
    system_wrapper(std::in_place_type_t<sys>, args &&...arguments)
        : m_underlying{std::in_place_type<sys>, std::forward<args>(arguments)...} {
        check_system_types<sys>();
    }

    template<sys_type type, typename... args>
    void call_method(args &&...arguments) {
        if constexpr(type == sys_type::start) {
            assert(m_start_callback && "Not a start system");
            m_start_callback(std::forward<args>(arguments)...);
        }
        if constexpr(type == sys_type::update) {
            assert(m_update_callback && "Not an update system");
            m_update_callback(std::forward<args>(arguments)...);
        }
        if constexpr(type == sys_type::cleanup) {
            assert(m_cleanup_callback && "Not a cleanup system");
            m_cleanup_callback(std::forward<args>(arguments)...);
        }
        if constexpr(type == sys_type::render) {
            assert(m_render_callback && "Not a render system");
            m_render_callback(std::forward<args>(arguments)...);
        }
    }

    template<sys_type type>
    [[nodiscard]] bool is_type() const {
        return m_types & static_cast<decltype(m_types)>(type);
    }

    auto types() const {
        return m_types;
    }

private:
    template<typename sys>
    void check_system_types() {
        auto *object = std::any_cast<std::decay_t<sys>>(&m_underlying);
        if constexpr(is_update_system<sys, context>) {
            m_update_callback = [object](seconds delta, context &ctx) {
                object->update(delta, ctx);
            };
            m_types |= static_cast<decltype(m_types)>(sys_type::update);
        }
        if constexpr(is_start_system<sys, context>) {
            m_start_callback = [object](context &ctx) {
                object->start(ctx);
            };
            m_types |= static_cast<decltype(m_types)>(sys_type::start);
        }
        if constexpr(is_cleanup_system<sys, context>) {
            m_cleanup_callback = [object](context &ctx) {
                object->cleanup(ctx);
            };
            m_types |= static_cast<decltype(m_types)>(sys_type::cleanup);
        }
        if constexpr(is_render_system<sys, context>) {
            m_render_callback = [object](context &ctx) {
                object->render(ctx);
            };
            m_types |= static_cast<decltype(m_types)>(sys_type::render);
        }
    }

    std::underlying_type_t<sys_type> m_types{};
    std::any m_underlying;
    std::function<void(seconds, context &)> m_update_callback;
    std::function<void(context &)> m_start_callback;
    std::function<void(context &)> m_cleanup_callback;
    std::function<void(context &)> m_render_callback;
};
} // namespace st
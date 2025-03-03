module;

#include <any>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

export module stay3.ecs:system_wrapper;

import stay3.core;
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
#define CHECK_AND_CALL(role) \
    if constexpr(type == sys_type::role) { \
        assert(m_##role##_callback && "Not " #role " system"); \
        m_##role##_callback(std::forward<args>(arguments)...); \
    }
        CHECK_AND_CALL(start)
        CHECK_AND_CALL(update)
        CHECK_AND_CALL(cleanup)
        CHECK_AND_CALL(render)
    }

private:
    template<typename sys>
    void check_system_types() {
        auto *object = std::any_cast<std::decay_t<sys>>(&m_underlying);
        if constexpr(is_update_system<sys, context>) {
            m_update_callback = [object](seconds delta, context &ctx) {
                object->update(delta, ctx);
            };
        }
        if constexpr(is_start_system<sys, context>) {
            m_start_callback = [object](context &ctx) {
                object->start(ctx);
            };
        }
        if constexpr(is_cleanup_system<sys, context>) {
            m_cleanup_callback = [object](context &ctx) {
                object->cleanup(ctx);
            };
        }
        if constexpr(is_render_system<sys, context>) {
            m_render_callback = [object](context &ctx) {
                object->render(ctx);
            };
        }
    }

    std::any m_underlying;
    std::function<void(seconds, context &)> m_update_callback;
    std::function<void(context &)> m_start_callback;
    std::function<void(context &)> m_cleanup_callback;
    std::function<void(context &)> m_render_callback;
};
} // namespace st
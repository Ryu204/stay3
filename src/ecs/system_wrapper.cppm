module;

#include <any>
#include <cassert>
#include <functional>
#include <type_traits>
#include <utility>

export module stay3.ecs:system_wrapper;

import stay3.core;
import stay3.input;
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
    sys_run_result call_method(args &&...arguments) {
#define CHECK_AND_CALL(role) \
    if constexpr(type == sys_type::role) { \
        assert(m_##role##_callback && "Not " #role " system"); \
        return m_##role##_callback(std::forward<args>(arguments)...); \
    }
        CHECK_AND_CALL(start)
        CHECK_AND_CALL(update)
        CHECK_AND_CALL(cleanup)
        CHECK_AND_CALL(render)
        CHECK_AND_CALL(post_update)
        CHECK_AND_CALL(input)
    }
#undef CHECK_AND_CALL

private:
    template<typename sys>
    void check_system_types() {
        auto *object = std::any_cast<std::decay_t<sys>>(&m_underlying);

        // Update
        if constexpr(is_update_system<sys, context>) {
            using return_t = decltype(std::declval<sys>().update(std::declval<seconds>(), std::declval<context &>()));
            if constexpr(std::is_convertible_v<return_t, sys_run_result>) {
                m_update_callback = [object](seconds delta, context &ctx) -> sys_run_result {
                    return object->update(delta, ctx);
                };
            } else {
                m_update_callback = [object](seconds delta, context &ctx) -> sys_run_result {
                    object->update(delta, ctx);
                    return sys_run_result::noop;
                };
            }
        }

        // Post update
        if constexpr(is_post_update_system<sys, context>) {
            using return_t = decltype(std::declval<sys>().post_update(std::declval<seconds>(), std::declval<context &>()));
            if constexpr(std::is_convertible_v<return_t, sys_run_result>) {
                m_post_update_callback = [object](seconds delta, context &ctx) -> sys_run_result {
                    return object->post_update(delta, ctx);
                };
            } else {
                m_post_update_callback = [object](seconds delta, context &ctx) -> sys_run_result {
                    object->post_update(delta, ctx);
                    return sys_run_result::noop;
                };
            }
        }

        // Start
        if constexpr(is_start_system<sys, context>) {
            using return_t = decltype(std::declval<sys>().start(std::declval<context &>()));
            if constexpr(std::is_convertible_v<return_t, sys_run_result>) {
                m_start_callback = [object](context &ctx) -> sys_run_result {
                    return object->start(ctx);
                };
            } else {
                m_start_callback = [object](context &ctx) -> sys_run_result {
                    object->start(ctx);
                    return sys_run_result::noop;
                };
            }
        }

        // Render
        if constexpr(is_render_system<sys, context>) {
            using return_t = decltype(std::declval<sys>().render(std::declval<context &>()));
            if constexpr(std::is_convertible_v<return_t, sys_run_result>) {
                m_render_callback = [object](context &ctx) -> sys_run_result {
                    return object->render(ctx);
                };
            } else {
                m_render_callback = [object](context &ctx) -> sys_run_result {
                    object->render(ctx);
                    return sys_run_result::noop;
                };
            }
        }

        // Cleanup
        if constexpr(is_cleanup_system<sys, context>) {
            using return_t = decltype(std::declval<sys>().cleanup(std::declval<context &>()));
            if constexpr(std::is_convertible_v<return_t, sys_run_result>) {
                m_cleanup_callback = [object](context &ctx) -> sys_run_result {
                    return object->cleanup(ctx);
                };
            } else {
                m_cleanup_callback = [object](context &ctx) -> sys_run_result {
                    object->cleanup(ctx);
                    return sys_run_result::noop;
                };
            }
        }

        // Input
        if constexpr(is_input_system<sys, context>) {
            using return_t = decltype(std::declval<sys>().input(std::declval<const event &>(), std::declval<context &>()));
            if constexpr(std::is_convertible_v<return_t, sys_run_result>) {
                m_input_callback = [object](const event &ev, context &ctx) -> sys_run_result {
                    return object->input(ev, ctx);
                };
            } else {
                m_input_callback = [object](const event &ev, context &ctx) -> sys_run_result {
                    object->input(ev, ctx);
                    return sys_run_result::noop;
                };
            }
        }
    }

    std::any m_underlying;
    std::function<sys_run_result(seconds, context &)> m_update_callback;
    std::function<sys_run_result(context &)> m_start_callback;
    std::function<sys_run_result(context &)> m_cleanup_callback;
    std::function<sys_run_result(context &)> m_render_callback;
    std::function<sys_run_result(seconds, context &)> m_post_update_callback;
    std::function<sys_run_result(const event &, context &)> m_input_callback;
};
} // namespace st
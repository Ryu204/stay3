module;

#include <cstdint>
#include <type_traits>

export module stay3.ecs:system_data;

import stay3.system;

export namespace st {

template<typename sys, typename context>
concept is_update_system = requires(sys &system, seconds delta, context &ctx) {
    system.update(delta, ctx);
};
template<typename sys, typename context>
concept is_start_system = requires(sys &system, context &ctx) {
    system.start(ctx);
};
template<typename sys, typename context>
concept is_cleanup_system = requires(sys &system, context &ctx) {
    system.cleanup(ctx);
};
template<typename sys, typename context>
concept is_render_system = requires(sys &system, context &ctx) {
    system.render(ctx);
};
/**
 *@brief When to execute a system
 */
enum class sys_type : std::uint8_t {
    update = 1 << 1,
    start = 1 << 2,
    cleanup = 1 << 3,
    render = 1 << 4,
};

/**
 * @note System with lowest priority runs in the end
 */
enum class sys_priority : std::uint8_t {
    very_high = 5,
    high = 4,
    medium = 3,
    low = 2,
    very_low = 1,
};
using sys_custom_priority = std::underlying_type_t<sys_priority>;
} // namespace st
module;

#include <cstdint>
#include <type_traits>

export module stay3.ecs:system_data;

import stay3.core;

export namespace st {

/**
 *@brief When to execute a system
 */
enum class sys_type : std::uint8_t {
    update = 1 << 1,
    start = 1 << 2,
    cleanup = 1 << 3,
    render = 1 << 4,
};
using sys_type_t = std::underlying_type_t<sys_type>;

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

template<typename sys, typename context>
constexpr auto sys_types = []() {
    sys_type_t result{};
#define CHECK_ROLE(role) \
    if constexpr(is_##role##_system<sys, context>) { result |= static_cast<sys_type_t>(sys_type::role); }

    CHECK_ROLE(update)
    CHECK_ROLE(start)
    CHECK_ROLE(render)
    CHECK_ROLE(cleanup)

    return result;
}();

template<sys_type type, typename sys, typename context>
constexpr bool is_system_type = sys_types<sys, context> & static_cast<sys_type_t>(type);

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
using sys_priority_t = std::underlying_type_t<sys_priority>;
} // namespace st
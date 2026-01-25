module;

#include <cstdint>
#include <type_traits>
export module stay3.system.script.angelscript:lifecycle_method;

export namespace st::ags {
enum class lifecycle_method : std::uint8_t {
    on_attached = 1 << 0,
    on_detached = 1 << 1,
    start = 1 << 2,
    update = 1 << 3,
    post_update = 1 << 4,
    input = 1 << 5,
};
using lifecycle_method_bitmask = std::underlying_type_t<lifecycle_method>;
static_assert(std::is_unsigned_v<lifecycle_method_bitmask>, "Life cycle method must be enum with unsigned type");
} // namespace st::ags
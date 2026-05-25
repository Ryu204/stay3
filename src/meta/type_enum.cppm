module;

#include <cstdint>

export module stay3.meta:type_enum;

namespace st::meta {
enum class type_enum : std::uint8_t {
    node,
    tree_context,
};
} // namespace st::meta
module;

#include <cstdint>
#include <type_traits>

export module stay3.ecs:component;

import :entity;

export namespace st {

template<typename type>
concept component = !std::is_same_v<entity, std::decay_t<type>>;

enum class comp_event : std::uint8_t {
    construct,
    destroy,
    update,
};

} // namespace st
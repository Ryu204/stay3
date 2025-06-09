module;

#include <cstdint>
#include <string>

export module stay3.graphics.text:text;

import stay3.ecs;
import :font;

namespace st {
export struct text {
    enum class align : std::uint8_t {
        left,
        right,
        center,
    };

    static constexpr font::size_type default_size{30};
    std::u8string content;
    font::size_type size{default_size};
    component_ref<font> font;
    align alignment{align::center};
};
} // namespace st
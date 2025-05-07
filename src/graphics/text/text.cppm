module;

#include <string>

export module stay3.graphics.text:text;

import stay3.ecs;
import :font;

namespace st {
export struct text {
    static constexpr font::size_type default_size{30};
    std::u8string content;
    font::size_type size{default_size};
    component_ref<font> font;
};
} // namespace st
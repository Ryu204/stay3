module;

#include <unordered_map>

export module stay3.system.text:font_state;

import stay3.graphics;
import stay3.ecs;
import :font_atlas;

export namespace st {
struct font_state {
    std::unordered_map<font::size_type, component_ref<font_atlas>> atlas_holders;
};
} // namespace st
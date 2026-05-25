module;

#include <cstdint>
#include <entt/entt.hpp>

export module stay3.meta:node;

import stay3.node;
import :type_enum;

namespace st::meta {
export void add_node_module_meta() {
    entt::meta_factory<tree_context>{}
        .type(
            static_cast<std::uint32_t>(type_enum::tree_context), "tree_context");

    auto type = entt::resolve<tree_context>();
}
}; // namespace st::meta
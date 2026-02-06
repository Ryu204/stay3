module;

#include <cassert>
#include <angelscript.h>

export module stay3.system.script.angelscript:register_tree_context;

import stay3.ecs;
import stay3.node;
import :engine;
import :register_type;

namespace st::ags {
export void register_tree_context(ags_engine &engine) {
    const auto *class_name = "TreeContext";
    auto res = engine->RegisterObjectType(class_name, sizeof(tree_context), asOBJ_REF | asOBJ_NOCOUNT);
    assert(res >= 0 && "Failed to register tree_context");

    res = engine->RegisterObjectMethod(class_name, "node get_node(Entity en)", asMETHODPR(tree_context, get_node, (entity), node &), asCALL_THISCALL);
    assert(res >= 0 && "Failed to register get_node");
}

export template<>
struct register_type<tree_context>: set_func_arg_object_mixin<tree_context> {
};
static_assert(is_registered_type<tree_context>);
} // namespace st::ags
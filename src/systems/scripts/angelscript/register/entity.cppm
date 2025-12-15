module;

#include <cassert>
#include <angelscript.h>

export module stay3.system.script.angelscript:register_entity;

import stay3.ecs;
import :engine;

namespace st::ags {
void register_entity(ags_engine &engine) {
    const auto res = engine->RegisterObjectType("Entity", sizeof(entity), asOBJ_VALUE | asOBJ_POD);
    assert(res >= 0 && "Failed to register entity");
    static entity null_entity;
    const auto other_res = engine->RegisterGlobalProperty("const Entity EntityNull", &null_entity);
    assert(other_res >= 0 && "Failed to register null entity");
}
} // namespace st::ags
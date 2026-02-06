module;

#include <cassert>
#include <angelscript.h>
#include <autowrapper/aswrappedcall.h>

export module stay3.system.script.angelscript:register_entity;

import stay3.ecs;
import :engine;
import :register_type;

namespace st::ags {

void entity_ctor(void *mem) {
    new(mem) entity;
}

void entity_dtor(void *mem) {
    static_cast<entity *>(mem)->~entity();
}

export void register_entity(ags_engine &engine) {
    // The ALLINTS flag is apparently needed:
    // https://www.gamedev.net/forums/topic/611415-angelscript-2212-released/4865940/
    auto res = engine->RegisterObjectType(
        "Entity", sizeof(entity),
        asOBJ_VALUE | asGetTypeTraits<entity>() | asOBJ_APP_CLASS_MORE_CONSTRUCTORS | asOBJ_APP_CLASS_ALLINTS);
    assert(res >= 0 && "Failed to register entity");
    static entity null_entity;
    res = engine->RegisterGlobalProperty("const Entity EntityNull", &null_entity);
    assert(res >= 0 && "Failed to register null entity");
    res = engine->RegisterObjectBehaviour("Entity", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(entity_ctor), asCALL_CDECL_OBJLAST);
    assert(res >= 0 && "Failed to register entity ctor");
    res = engine->RegisterObjectBehaviour("Entity", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(entity_dtor), asCALL_CDECL_OBJLAST);
    assert(res >= 0 && "Failed to register entity dtor");
    res = engine->RegisterObjectMethod("Entity", "Entity& opAssign(const Entity &in)", asMETHODPR(entity, operator=, (const entity &), entity &), asCALL_THISCALL);
    assert(res >= 0 && "Failed to register entity assignment");
}

export template<>
struct register_type<entity>: set_func_arg_object_mixin<entity> {
};
static_assert(is_registered_type<entity>);
} // namespace st::ags
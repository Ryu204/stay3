module;

#include <cassert>
#include <angelscript.h>
export module stay3.system.script.angelscript:register_transform;

import stay3.core;
import :engine;
import :register_type;

namespace st::ags {

export void register_transform(ags_engine &engine) {
    const auto *class_name = "Transform";
    auto res = engine->RegisterObjectType(class_name, sizeof(transform), asOBJ_REF);
}

} // namespace st::ags
module;

#include <cassert>

export module stay3.system.script.angelscript:register_all;

import stay3.ecs;
import :engine;
export import :register_entity;
export import :register_logger;
export import :register_tree_context;
export import :register_primitives;
export import :register_vector;
export import :register_transform;
export import :register_assertion;

namespace st::ags {
void register_all_types(ags_engine &engine, bool register_assert) {
    register_entity(engine);
    register_logger(engine);
    register_tree_context(engine);
    if(register_assert) {
        register_assertion(engine);
    }
    register_all_vectors(engine);
    // register_transform(engine);
}
} // namespace st::ags
module;

#include <cassert>

export module stay3.system.script.angelscript:register_all;

import stay3.ecs;
import :engine;
import :register_entity;
import :register_logger;

namespace st::ags {
void register_all_types(ags_engine &engine) {
    register_entity(engine);
    register_logger(engine);
}
} // namespace st::ags
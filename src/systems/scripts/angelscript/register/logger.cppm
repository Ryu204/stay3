module;

#include <cassert>
#include <iostream>
#include <string>
#include <angelscript.h>

export module stay3.system.script.angelscript:register_logger;

import stay3.ecs;
import stay3.core;
import :engine;

namespace st::ags {

void temporary_print(const std::string &message) {
    std::cout << "Script says: " << message << '\n';
}

void register_logger(ags_engine &engine) {
    const auto res = engine->RegisterGlobalFunction("void print(const string&in message)", asFUNCTION(temporary_print), asCALL_CDECL);
    assert(res >= 0 && "Failed to register entity");
}
} // namespace st::ags
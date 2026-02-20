module;

#include <cassert>
#include <string>
#include <angelscript.h>
#include <autowrapper/aswrappedcall.h>

export module stay3.system.script.angelscript:register_assertion;

import stay3.system.script;
import stay3.core;
import :engine;

namespace st::ags {
void script_assertion(bool condition, const std::string &message) {
    if(!condition) {
        log::error("Script assertion: ", message, '.');
        throw script_error{message};
    }
}

void script_assertion_no_message(bool condition) {
    if(!condition) {
        log::error("Script assertion: untitled error.");
        throw script_error{"Untitled error"};
    }
}

export void register_assertion(ags_engine &engine) {
    auto res = engine->RegisterGlobalFunction("void assert(bool, const string &in message)", asFUNCTION(script_assertion), asCALL_CDECL);
    assert(res >= 0 && "Failed to register assert with message");
    res = engine->RegisterGlobalFunction("void assert(bool)", asFUNCTION(script_assertion_no_message), asCALL_CDECL);
    assert(res >= 0 && "Failed to register assert without message");
}
} // namespace st::ags
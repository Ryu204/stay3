module;

#include <angelscript.h>

export module stay3.system.script.angelscript:engine;
import stay3.system.script;
import stay3.core;

namespace st {
export class ags_engine {
public:
    ags_engine(): engine{asCreateScriptEngine()} {
        if(engine == nullptr) {
            throw st::script_error{"Failed to create angelscript engine"};
        }
        default_context = engine->CreateContext();
        if(default_context == nullptr) {
            throw st::script_error{"Failed to create engine context"};
        }
    }
    ~ags_engine() {
        default_context->Release();
        const auto release_res = engine->ShutDownAndRelease();
        if(release_res < 0) {
            log::error("Failed to shutdown anglescript engine");
        }
    }
    ags_engine(const ags_engine &) = delete;
    ags_engine(ags_engine &&) noexcept = delete;
    ags_engine &operator=(const ags_engine &) = delete;
    ags_engine &operator=(ags_engine &&) noexcept = delete;

    [[nodiscard]] const asIScriptEngine *operator->() const {
        return engine;
    }

    asIScriptEngine *operator->() {
        return engine;
    }

    asIScriptEngine *get() {
        return engine;
    }

    asIScriptContext *context() {
        return default_context;
    }

private:
    asIScriptEngine *engine;
    asIScriptContext *default_context;
};
} // namespace st
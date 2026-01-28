module;

#include <cstddef>
#include <angelscript.h>

export module stay3.system.script.angelscript:register_primitives;

import :register_type;

namespace st::ags {
export template<>
struct register_type<float> {
    static int set_func_arg(asIScriptContext &ctx, std::size_t index, float value) {
        return ctx.SetArgFloat(index, value);
    }
};
static_assert(is_registered_type<float>);
} // namespace st::ags
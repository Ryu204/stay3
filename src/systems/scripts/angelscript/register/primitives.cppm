module;

#include <cstddef>
#include <angelscript.h>

export module stay3.system.script.angelscript:register_primitives;

import :register_type;

// TODO: Currently, we have not supported primitives reference syntax. It requires
// a call to `asIScriptContext::SetArgAddress`.
//
// From the perspective of `exec`'s caller, they need a way to say "This argument is
// passed as reference". I intend to do that via a template, i.e to indicate a parameter
// is passed as reference we pass it as `Ref<Value> as_ref(value)`.
namespace st::ags {
export template<>
struct register_type<float> {
    static int set_func_arg(asIScriptContext &ctx, std::size_t index, float value) {
        return ctx.SetArgFloat(index, value);
    }
};
static_assert(is_registered_type<float>);
} // namespace st::ags
module;

#include <concepts>
#include <cstddef>
#include <angelscript.h>

export module stay3.system.script.angelscript:register_type;

namespace st::ags {

export template<typename T>
struct set_func_arg_object_mixin {
    static int set_func_arg(asIScriptContext &ctx, std::size_t index, T &value) {
        return ctx.SetArgObject(index, &value);
    }
};

export template<typename T>
struct register_type {};

template<typename T>
struct registered_type {};

template<typename T>
struct registered_type<register_type<T>> {
    using type = T;
};

template<typename T>
concept is_register_type = requires(std::size_t index, registered_type<T>::type &val, asIScriptContext &ctx) {
    { T::set_func_arg(ctx, index, val) } -> std::same_as<int>;
};

export template<typename T>
concept is_registered_type = is_register_type<register_type<T>>;

} // namespace st::ags
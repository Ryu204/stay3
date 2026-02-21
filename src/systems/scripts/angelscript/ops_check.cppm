module;

#include <array>
#include <format>
#include <angelscript.h>

export module stay3.system.script.angelscript:ops_check;

import stay3.system.script;

import :objects;
import :register_all;
import :register_type;

namespace st::ags {

export bool check_call(int result) {
    return result >= 0;
}

export scripts_operation_result check_exec(asIScriptContext &ctx, int exec_result) {
    if(exec_result != asEXECUTION_FINISHED) {
        if(exec_result == asEXECUTION_EXCEPTION) {
            return {
                .error_message = std::format("Exception occured: {}", ctx.GetExceptionString()),
                .is_ok = false,
            };
        }
        return {
            .error_message = std::format("Execution error, code {}. Check angelscript docs for more info.", exec_result),
            .is_ok = false,
        };
    }
    return {.is_ok = true};
}

export scripts_operation_result exec(asIScriptContext &ctx, function &free_func) {
    auto *raw_func = free_func.get();
    if(!check_call(ctx.Prepare(raw_func))) {
        return {
            .error_message = "Failed to prepare context",
            .is_ok = false,
        };
    }
    const auto exec_result = ctx.Execute();
    if(const auto check_result = check_exec(ctx, exec_result); !check_result.is_ok) {
        return check_result;
    }
    return {.is_ok = true};
}

export template<typename... args>
scripts_operation_result exec(asIScriptContext &ctx, function &method, object &instance, args &&...arguments) {
    auto *raw_method = method.get();
    auto *raw_instance = instance.get();
    if(!check_call(ctx.Prepare(raw_method))) {
        return {
            .error_message = "Failed to prepare context",
            .is_ok = false,
        };
    }
    if(!check_call(ctx.SetObject(raw_instance))) {
        return {
            .error_message = "Failed to set \"this\" (? idk if in Angelscript it's also called this) object",
            .is_ok = false,
        };
    }
    constexpr auto argc = sizeof...(args);
    if(argc > 0) {
        using storage_t = std::tuple<
            std::conditional_t<std::is_lvalue_reference_v<args>, args, std::decay_t<args>>...>;
        storage_t storage{std::forward<args>(arguments)...};
        const auto has_error = std::apply(
            [&ctx](auto &...tup_elem) -> std::optional<std::size_t> {
                std::size_t idx = 0;
                std::array<bool, argc> ok = {check_call(
                    register_type<std::decay_t<args>>::set_func_arg(ctx, idx++, tup_elem))...};
                for(std::size_t i = 0; i < argc; ++i) {
                    if(!ok[i]) { return i; };
                }
                return std::nullopt;
            },
            storage);
        if(has_error) {
            return {
                .error_message = std::format("Failed to set {}-th argument (0-based)", has_error.value()),
                .is_ok = false,
            };
        }
    }
    const auto exec_result = ctx.Execute();
    if(const auto check_result = check_exec(ctx, exec_result); !check_result.is_ok) {
        return check_result;
    }
    return {.is_ok = true};
}

} // namespace st::ags
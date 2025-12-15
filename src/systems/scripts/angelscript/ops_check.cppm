module;

#include <format>
#include <angelscript.h>

export module stay3.system.script.angelscript:ops_check;

import stay3.system.script;

namespace st {

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

export scripts_operation_result exec(asIScriptContext &ctx, asIScriptFunction &free_func) {
    if(!check_call(ctx.Prepare(&free_func))) {
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
scripts_operation_result exec(asIScriptContext &ctx, asIScriptFunction &method, asIScriptObject &instance, args &&...arguments) {
    if(!check_call(ctx.Prepare(&method))) {
        return {
            .error_message = "Failed to prepare context",
            .is_ok = false,
        };
    }
    if(!check_call(ctx.SetObject(&instance))) {
        return {
            .error_message = "Failed to set \"this\" (? idk if in Angelscript it's also called this) object",
            .is_ok = false,
        };
    }
    {
        std::size_t arg_order = 0;
        const auto ok = (check_call(ctx.SetArgObject(arg_order++, std::addressof(std::forward<args>(arguments)))) && ...);
        if(!ok) {
            return {
                .error_message = std::format("Failed to set {}-th argument (0-based)", arg_order - 1),
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

} // namespace st
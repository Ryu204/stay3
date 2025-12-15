module;

#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <angelscript.h>

export module stay3.system.script.angelscript:entity_scripts_runner;

import stay3.system.script;
import stay3.ecs;
import :objects;
import :ops_check;
namespace st::ags {
export class entity_scripts_runner {
private:
    entity en;
    std::unordered_map<script_id, object> instances;

public:
    entity_scripts_runner(entity en): en{en} {}

    scripts_operation_result attach_component_type(script_id id, function &factory, function &on_attached, asIScriptContext &context) {
        assert(!instances.contains(id) && "This component type was already added");
        if(const auto check_result = exec(context, *factory.get()); !check_result.is_ok) {
            return check_result;
        }
        auto *raw_instance = *(static_cast<asIScriptObject **>(context.GetAddressOfReturnValue()));
        if(raw_instance == nullptr) {
            return {
                .error_message = "Factory did not return an instance, maybe it is not a factory?",
                .is_ok = false,
            };
        }
        object instance{raw_instance};
        if(const auto check_result = exec(context, *on_attached.get(), *raw_instance, en); !check_result.is_ok) {
            return check_result;
        }
        auto &&[iter, ok] = instances.emplace(id, std::move(instance));
        assert(ok && "Cannot emplace new instance");

        return {.is_ok = true};
    }

    scripts_operation_result detach_componnet_type(int type) {
        return {
            .error_message = "unimplemented",
            .is_ok = false,
        };
    }

    [[nodiscard]] std::uint32_t size() const {
        return instances.size();
    }

    template<sys_type type, typename... args>
    scripts_operation_result run(args &&...arguments) {
        if constexpr(type == sys_type::start) {}
    }
};
} // namespace st::ags
module;

#include <cassert>
#include <exception>
#include <filesystem>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <sol/sol.hpp>

export module stay3.system.script.lua;
import stay3.system.script;
import stay3.node;
import stay3.ecs;

import :global_fields;

sol::function component_batch_call_func(std::string_view method, sol::state &state, const sol::environment &env) {
    constexpr std::string_view unformatted_script = R"(
return function(entities_data, ...)
    for entity, data in pairs(entities_data) do
        local comps = data["{0}"]
        for comp_type, comp_instance in pairs(comps) do
            comp_instance["{1}"](comp_instance, ...)
        end
    end
end
)";
    const std::string script = std::format(
        unformatted_script,
        st::lua_field_entities_data_components,
        method);
    const auto result = state.safe_script(script);
    if(!result.valid()) {
        throw sol::error{result};
    }
    assert(result.get_type() == sol::type::function && "Script must return a function");
    const sol::function func{result};
    const auto set_env_result = sol::set_environment(env, func);
    assert(set_env_result && "Failed to set script execution environment");
    return func;
}

export namespace st {

using lua_scripts = scripts<script_lang::lua>;

struct lua_script_system_config {
    std::filesystem::path component_script;
};

using lua_script_manager = script_manager<script_lang::lua>;

class lua_script_system: public script_system<script_lang::lua> {
public:
    lua_script_system(lua_script_system_config config): system_config{std::move(config)} {
    }

protected:
    [[nodiscard]] scripts_operation_result initialize() override {
        try {
            maybe_state = sol::state{};
            environment = sol::environment{*maybe_state, maybe_state->globals()};
            // Needed for `setmetatable`
            maybe_state->open_libraries(sol::lib::base);
            entities_data = maybe_state->create_table();
            const auto maybe_component_type = maybe_state->safe_script_file(system_config.component_script.string(), sol::load_mode::text);
            auto validation_result = validate_base_component(maybe_component_type);
            if(!validation_result.is_valid) {
                return {.error_message = std::string("Invalid base component: ") + validation_result.error_message.value_or("No more details"), .is_ok = false};
            }
            component_type = maybe_component_type;
            environment.set(lua_field_component_base, component_type);

            batch_methods[lua_field_component_base_start] = component_batch_call_func(lua_field_component_base_start, *maybe_state, environment);
            batch_methods[lua_field_component_base_update] = component_batch_call_func(lua_field_component_base_update, *maybe_state, environment);
            batch_methods[lua_field_component_base_post_update] = component_batch_call_func(lua_field_component_base_post_update, *maybe_state, environment);
            batch_methods[lua_field_component_base_input] = component_batch_call_func(lua_field_component_base_input, *maybe_state, environment);
            batch_methods[lua_field_component_base_on_destroy] = component_batch_call_func(lua_field_component_base_on_destroy, *maybe_state, environment);

            return {
                .is_ok = true,
            };
        } catch(std::exception &e) {
            return {.error_message = e.what(), .is_ok = false};
        }
    }

    [[nodiscard]] scripts_operation_result shutdown() override {
        try {
            batch_methods.clear();
            entities_data.reset();
            environment.reset();
            component_type.reset();
            registered_scripts.clear();
            maybe_state.reset();
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
        return {.is_ok = true};
    }
    [[nodiscard]] script_validation_result load_script(const script_system::path &filepath, script_id script_id) override {
        try {
            auto &&lua = maybe_state.value();
            auto maybe_lua_component = lua.safe_script_file(filepath.string(), environment, sol::load_mode::text);
            if(!maybe_lua_component.valid()) {
                return {
                    .error_message = std::string("Invalid component definition: ") + sol::error{maybe_lua_component}.what(),
                    .is_valid = false};
            }
            const auto validation_result = validate_component(maybe_lua_component);
            if(validation_result.is_valid) {
                register_script(std::move(maybe_lua_component), script_id);
            }
            return validation_result;
        } catch(std::exception &e) {
            return {.error_message = e.what(), .is_valid = false};
        }
    };
    [[nodiscard]] scripts_operation_result update_all_scripts(float dt) override {
        try {
            const auto result = batch_methods[lua_field_component_base_update](entities_data, dt);
            return check_op_result(result);
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
    }
    [[nodiscard]] scripts_operation_result post_update_all_scripts(float dt) override {
        try {
            const auto result = batch_methods[lua_field_component_base_post_update](entities_data, dt);
            return check_op_result(result);
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
    };
    [[nodiscard]] scripts_operation_result input_all_scripts() override {
        try {
            const auto result = batch_methods[lua_field_component_base_input](entities_data);
            return check_op_result(result);
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
    };

    // TODO: on_destroy and start method

    [[nodiscard]] scripts_operation_result attach_script(entity en, script_id script_id) override {
        try {
            const auto en_num = en.numeric();
            if(entities_data[en_num] == sol::nil) {
                auto data = maybe_state->create_table();
                data[lua_field_entities_data_components] = maybe_state->create_table();
                entities_data.set(en_num, data);
            }
            sol::table this_entity_components = entities_data[en_num][lua_field_entities_data_components];

            const auto &type = registered_scripts[script_id];
            assert(this_entity_components[type] == sol::nil && "Entity already has this component as recorded by lua system");
            const auto new_ctor = type.get<sol::function>("new");
            if(!environment.set_on(new_ctor)) {
                return {
                    .error_message = "Failed to set \"new\" constructor execution environment",
                    .is_ok = false,
                };
            }
            auto raw_instance = new_ctor(type);
            if(!raw_instance.valid()) {
                return {
                    .error_message = std::string("Failed to create new script component instance via new: ") + sol::error{raw_instance}.what(),
                    .is_ok = false,
                };
            }
            if(raw_instance.get_type() != sol::type::table) {
                return {
                    .error_message = "Component's \"new\" method must return a table",
                    .is_ok = false,
                };
            }
            const sol::table instance{raw_instance};
            try {
                const auto result = instance.get<sol::function>(lua_field_component_base_on_attach)(instance, en_num);
                if(!result.valid()) {
                    throw sol::error{result};
                }
            } catch(std::exception &e) {
                return {
                    .error_message = std::string{"Failed to call "} + lua_field_component_base_on_attach + ": " + e.what(),
                    .is_ok = false,
                };
            }
            this_entity_components.set(type, instance);

            try {
                const auto result = instance.get<sol::function>(lua_field_component_base_start)(instance);
                if(!result.valid()) {
                    throw sol::error{result};
                }
            } catch(std::exception &e) {
                return {
                    .error_message = std::format("Failed to call {}: {}", lua_field_component_base_start, e.what()),
                    .is_ok = false,
                };
            }

            return {.is_ok = true};
        } catch(std::exception &e) {
            return {.error_message = e.what(), .is_ok = false};
        }
    }
    [[nodiscard]] scripts_operation_result detach_script(entity en, script_id script_id) override {
        try {
            assert(registered_scripts.contains(script_id) && "Not a registered script id");
            sol::table script_type = registered_scripts.at(script_id);
            sol::table entity_components;
            sol::table instance;
            try {
                entity_components = entities_data[en.numeric()][lua_field_entities_data_components];
                instance = entity_components[script_type];
            } catch(std::exception &e) {
                return {
                    .error_message = std::format("Entity does not have component to detach"),
                    .is_ok = false,
                };
            }
            {
                const auto on_destroy_result = instance.get<sol::function>(lua_field_component_base_on_destroy)(instance);
                if(!on_destroy_result.valid()) {
                    return {
                        .error_message = std::format("On destroy handler: {}", sol::error{on_destroy_result}.what()),
                        .is_ok = false,
                    };
                }
            }
            {
                const auto on_detach_result = instance.get<sol::function>(lua_field_component_base_on_detach)(instance);
                if(!on_detach_result.valid()) {
                    return {
                        .error_message = std::format("On detach handler: {}", sol::error{on_detach_result}.what()),
                        .is_ok = false,
                    };
                }
            }
            entity_components[script_type] = sol::nil;
            return {.is_ok = true};
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
    }

private:
    const sol::state &state() {
        return maybe_state.value();
    }

    [[nodiscard]] script_validation_result validate_component(const sol::protected_function_result &maybe_comp) {
        if(!maybe_comp.valid()) {
            return {
                .error_message = sol::error{maybe_comp}.what(),
                .is_valid = false,
            };
        }
        if(maybe_comp.get_type() != sol::type::table) {
            return {
                .error_message = "Component must be a table",
                .is_valid = false};
        }
        const auto table = maybe_comp.get<sol::table>();
        const auto name = table[lua_field_component_name];
        if(name.get_type() != sol::type::string) {
            return {
                .error_message = "Missing or invalid \"name\" field",
                .is_valid = false,
            };
        }
        if(table[sol::metatable_key].get_type() != sol::type::table
           || table[sol::metatable_key].get<sol::table>() != component_type) {
            return {
                .error_message = "Component must set the base component table as metatable (i.e inherit from base component \"class\")",
                .is_valid = false,
            };
        }
        return {
            .is_valid = true,
            .name = name.get<script_name>(),
        };
    }

    [[nodiscard]] static script_validation_result validate_base_component(const sol::protected_function_result &maybe_comp) {
        if(!maybe_comp.valid()) {
            return {
                .error_message = sol::error{maybe_comp}.what(),
                .is_valid = false,
            };
        }
        if(maybe_comp.get_type() != sol::type::table) {
            return {
                .error_message = "Component must be a table",
                .is_valid = false};
        }
        sol::table table{maybe_comp};
        for(const auto &field: lua_field_component_base_fields) {
            if(table[field] == sol::nil) {
                return {
                    .error_message = std::string("Missing field: ") + field,
                    .is_valid = false,
                };
            }
        }
        return {.is_valid = true};
    }

    static scripts_operation_result check_op_result(const sol::protected_function_result &result) {
        if(!result.valid()) {
            return {
                .error_message = sol::error{result}.what(),
                .is_ok = false,
            };
        }
        return {.is_ok = true};
    }

    void register_script(sol::protected_function_result &&script, script_id script_id) {
        assert(!registered_scripts.contains(script_id) && "Id registered twice");
        auto &&[iter, ok] = registered_scripts.try_emplace(script_id, sol::table{std::move(script)});
        assert(ok);
    }

    lua_script_system_config system_config;
    std::optional<sol::state> maybe_state{std::nullopt};
    std::unordered_map<script_id, sol::table> registered_scripts;

    sol::table component_type;
    sol::environment environment;
    sol::table entities_data;
    std::unordered_map<const char *, sol::function> batch_methods;
};
} // namespace st
module;

#include <cassert>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <angelscript.h>
#include <scriptarray/scriptarray.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptdictionary/scriptdictionary.h>
#include <scriptstdstring/scriptstdstring.h>
export module stay3.system.script.angelscript;

import stay3.system.script;
import stay3.ecs;
import stay3.core;
import :engine;
import :objects;
import :entity_scripts_runner;
import :register_all;
import :ops_check;

namespace st {

export using ags_script_manager = script_manager<script_lang::angelscript>;
export using ags_scripts = scripts<script_lang::angelscript>;
export struct angelscript_system_config {
    std::filesystem::path base_script_path;
};

struct system_state {
    ags_engine engine;
    CScriptBuilder scripts_builder;
    std::optional<int> base_component_typeid{std::nullopt};
    [[nodiscard]] const auto *base_component_type_info() const {
        assert(base_component_typeid.has_value() && "Set base component id first");
        const auto *res = engine->GetTypeInfoById(*base_component_typeid);
        assert(res != nullptr);
        return res;
    }
};

script_validation_result build_module(const char *name, CScriptBuilder &builder, const std::filesystem::path &path, ags_engine &engine) {
    if(!check_call(builder.StartNewModule(engine.get(), name))) {
        return {
            .error_message = "Unrecoverable error while starting a new module.",
            .is_valid = false,
        };
    }
    {
        std::string filename = path.string();
        const char *c_filename = filename.c_str();
        if(!check_call(builder.AddSectionFromFile(c_filename))) {
            return {
                .error_message = "Invalid filename or invalid preprocessor in script",
                .is_valid = false,
            };
        }
    };
    if(!check_call(builder.BuildModule())) {
        return {
            .error_message = "Script contains error(s)",
            .is_valid = false,
        };
    }
    return {.is_valid = true};
}

struct load_base_component_result {
    std::decay_t<decltype(system_state::base_component_typeid)> type_index{std::nullopt};
    std::optional<std::string> error_message{std::nullopt};
    [[nodiscard]] bool is_ok() const {
        return type_index.has_value();
    }
};

load_base_component_result load_base_component(const std::filesystem::path &path, CScriptBuilder &builder, ags_engine &engine) {
    const auto build_mod_res = build_module("Stay3", builder, path, engine);
    if(!build_mod_res.is_valid) {
        return {
            .error_message = build_mod_res.error_message.value_or("Failed to build module"),
        };
    }
    const auto *mod = builder.GetModule();
    const auto type_count = mod->GetObjectTypeCount();
    for(int i = 0; i < type_count; ++i) {
        const auto *type_info = mod->GetObjectTypeByIndex(i);
        if(std::strcmp(type_info->GetName(), "Component") == 0) {
            return {.type_index = type_info->GetTypeId()};
        }
    }
    return {.error_message = "No type named \"Component\" found in base script"};
}

struct component_script_info {
    std::string name;
    ags::function factory;
    ags::function on_attached;
    ags::function on_detached;
};

export class angelscript_system: public script_system<script_lang::angelscript> {
private:
    std::unique_ptr<system_state> m_state;
    angelscript_system_config m_config;
    std::unordered_map<script_id, component_script_info> m_scripts_info;
    std::unordered_map<entity, ags::entity_scripts_runner, entity_hasher, entity_equal> m_script_runners;

    system_state &state() {
        assert(m_state && "Uninitialized or failed initialization");
        return *m_state;
    }
    [[nodiscard]] const std::string &get_module_name(script_id id) {
        auto iter = m_scripts_info.find(id);
        if(iter == m_scripts_info.end()) {
            auto &&[new_iter, ok] = m_scripts_info.emplace(id, std::format("Module{}", id));
            assert(ok && "Failed to emplace new module name");
            iter = new_iter;
        }
        return iter->second.name;
    }

public:
    angelscript_system(std::filesystem::path container_script_path): m_config{std::move(container_script_path)} {}

protected:
    [[nodiscard]] scripts_operation_result initialize() override {
        try {
            m_state = std::make_unique<system_state>();
            const auto error_callback = +[](const asSMessageInfo *msg, void *) {
                switch(msg->type) {
                case asEMsgType::asMSGTYPE_ERROR:
                    log::error("Angelscript: ", msg->section, ":", msg->row, ":", msg->col, ": ", msg->message);
                    break;
                case asEMsgType::asMSGTYPE_INFORMATION:
                    log::info("Angelscript: ", msg->section, ":", msg->row, ":", msg->col, ": ", msg->message);
                    break;
                case asEMsgType::asMSGTYPE_WARNING:
                    log::warn("Angelscript: ", msg->section, ":", msg->row, ":", msg->col, ": ", msg->message);
                    break;
                default:
                    log::warn("Unimplemented log level:");
                    log::info("Angelscript: ", msg->section, ":", msg->row, ":", msg->col, ": ", msg->message);
                    break;
                };
            };
            auto &engine = state().engine;
            if(!check_call(
                   engine->SetMessageCallback(asFUNCTION(error_callback), nullptr, asCALL_CDECL))) {
                return {
                    .error_message = "Failed to set engine error callback",
                    .is_ok = false,
                };
            }
            RegisterStdString(engine.get());
            RegisterScriptArray(engine.get(), true);
            RegisterScriptDictionary(engine.get());
            ags::register_all_types(engine);

            {
                const auto component_load_result = load_base_component(m_config.base_script_path, state().scripts_builder, engine);
                if(!component_load_result.is_ok()) {
                    return {
                        .error_message = std::format("Failed to load base component script:\n\t{}", component_load_result.error_message.value_or("No more detail")),
                        .is_ok = false,
                    };
                }
                state().base_component_typeid = component_load_result.type_index;
            }

            return {.is_ok = true};
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
    }

    [[nodiscard]] scripts_operation_result shutdown() override {
        m_script_runners.clear();
        m_scripts_info.clear();
        m_state.reset();
        return {.is_ok = true};
    }

    [[nodiscard]] script_validation_result load_script(const path &filepath, script_id script_id) override {
        try {
            CScriptBuilder sbuilder;
            const auto &module_name = get_module_name(script_id);
            if(!check_call(sbuilder.StartNewModule(state().engine.get(), module_name.c_str()))) {
                return {
                    .error_message = "Unrecoverable error while starting a new module.",
                    .is_valid = false,
                };
            }
            {
                std::string filename = filepath.string();
                const char *c_filename = filename.c_str();
                if(!check_call(sbuilder.AddSectionFromFile(c_filename))) {
                    return {
                        .error_message = "Invalid filename or invalid preprocessor in script",
                        .is_valid = false,
                    };
                }
            };
            if(!check_call(sbuilder.BuildModule())) {
                return {
                    .error_message = "Script contains error(s). Check log for more info.",
                    .is_valid = false,
                };
            }
            {
                // Inspect the module
                auto *mod = sbuilder.GetModule();
                const asUINT type_count = mod->GetObjectTypeCount();
                const auto *base_component_type_info = state().base_component_type_info();
                const asITypeInfo *component_derives_ti{nullptr};
                unsigned int component_derives_count = 0;
                for(asUINT i = 0; i < type_count; ++i) {
                    const auto *type_info = mod->GetObjectTypeByIndex(i);
                    if(type_info->GetTypeId() == base_component_type_info->GetTypeId()) {
                        continue;
                    }
                    if(type_info->DerivesFrom(base_component_type_info)) {
                        ++component_derives_count;
                        component_derives_ti = type_info;
                    }
                }
                if(component_derives_count != 1) {
                    return {
                        .error_message = std::format("More or less than 1 type ({} types) derive from Component in script", component_derives_count),
                        .is_valid = false,
                    };
                }
                assert(component_derives_ti != nullptr);
                const auto *name = component_derives_ti->GetName();
                {
                    // Cache type info
                    auto &saved_info = m_scripts_info.at(script_id);
                    const auto ctor_count = component_derives_ti->GetFactoryCount();
                    if(ctor_count <= 0) {
                        return {
                            .error_message = std::format("No factory method found on type \"{}\"", name),
                            .is_valid = false,
                        };
                    }
                    saved_info.factory.acquire(component_derives_ti->GetFactoryByIndex(0));

#define ACQUIRE_METHOD(var_name, script_name) \
    { \
        auto *(var_name) = component_derives_ti->GetMethodByName(#script_name); \
        if((var_name) == nullptr) { \
            return { \
                .error_message = "Missing " #script_name " method", \
                .is_valid = false, \
            }; \
        } \
        saved_info.var_name.acquire(component_derives_ti->GetMethodByName(#script_name)); \
    }

                    ACQUIRE_METHOD(on_attached, onAttached);
                    ACQUIRE_METHOD(on_detached, onDetached);

#undef ACQUIRE_METHOD
                }
                return {
                    .is_valid = true,
                    .name = name,
                };
            }
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_valid = false,
            };
        }
    }
    [[nodiscard]] scripts_operation_result update_all_scripts(float dt) override {
        return {
            .error_message = "Unimplemented",
            .is_ok = false,
        };
    }
    [[nodiscard]] scripts_operation_result post_update_all_scripts(float dt) override {
        return {
            .error_message = "Unimplemented",
            .is_ok = false,
        };
    }
    [[nodiscard]] scripts_operation_result input_all_scripts() override {
        return {
            .error_message = "Unimplemented",
            .is_ok = false,
        };
    }
    [[nodiscard]] scripts_operation_result attach_script(entity en, script_id script_id) override {
        if(!m_script_runners.contains(en)) {
            m_script_runners.emplace(en, en);
        }
        auto &runner = m_script_runners.at(en);
        auto &script_info = m_scripts_info.at(script_id);
        auto &context = state().engine.context();
        return runner.attach_component_type(script_id, script_info.factory, script_info.on_attached, context);
    }

    [[nodiscard]] scripts_operation_result detach_script(entity en, script_id script_id) override {
        return {
            .error_message = "Unimplemented",
            .is_ok = false,
        };
    }
};
} // namespace st
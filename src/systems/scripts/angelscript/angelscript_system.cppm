module;
#include <cassert>
#include <cstring>
#include <exception>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
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
import :lifecycle_method;
import :script_info;

namespace st {

void inspect_ags_type(const asITypeInfo *type) {
    const auto meth_count = type->GetMethodCount();
    std::cout << type->GetName() << '(' << meth_count << " methods)\n";
    for(auto i = 0; i < meth_count; ++i) {
        for(auto virt = 0; virt <= 1; ++virt) {
            const auto *meth = type->GetMethodByIndex(i, virt);
            std::cout << "Virtual: " << virt << '\n';
            std::cout << "\tName:" << meth->GetName() << "; Object name: " << meth->GetObjectName() << '\n';
            std::cout << "\tDeclaration: " << meth->GetDeclaration() << '\n';
            std::cout << "\tIs override: " << meth->IsOverride() << '\n';
        }
    }

    const auto *pu = type->GetMethodByDecl("void Bird::post_update()");
    if(pu == nullptr) {
        std::cout << "Bird::post_update is null\n";
    } else {
        std::cout << "\tName:" << pu->GetName() << "; Object name: " << pu->GetObjectName() << '\n';
        std::cout << "\tDeclaration: " << pu->GetDeclaration() << '\n';
        std::cout << "\tIs override: " << pu->IsOverride() << '\n';
    }
}

export using ags_script_manager = script_manager<script_lang::angelscript>;
export using ags_scripts = scripts<script_lang::angelscript>;
export struct angelscript_system_config {
    std::filesystem::path base_script_path;
};

struct system_state {
    ags_engine engine;
    CScriptBuilder scripts_builder;
    std::optional<int> base_component_typeid{std::nullopt};
    // We have not been able to retrieve the correct function pointers of lifecycle methods
    // of each component
    // We need to retrieve the pointer via `GetMethodByName` with `virtual = false`
    // Then, check if the returned pointer is the same as the one in base component
    // If it is, this component does not have a custom method of that lifecycle method,
    // And can be skipped
    //
    // However, we need to test with the case where there are at least 3 classes in
    // inheritance chain, because I am not sure how the virtual parameter affect the returned
    // result from `GetMethodByName`
    [[nodiscard]] const auto *base_component_type_info() const {
        assert(base_component_typeid.has_value() && "Set base component id first");
        const auto *res = engine->GetTypeInfoById(*base_component_typeid);
        assert(res != nullptr);
        return res;
    }
};

script_validation_result build_module(const char *name, CScriptBuilder &builder, const std::filesystem::path &path, ags_engine &engine) {
    if(!ags::check_call(builder.StartNewModule(engine.get(), name))) {
        return {
            .error_message = "Unrecoverable error while starting a new module.",
            .is_valid = false,
        };
    }
    {
        std::string filename = path.string();
        const char *c_filename = filename.c_str();
        if(!ags::check_call(builder.AddSectionFromFile(c_filename))) {
            return {
                .error_message = "Invalid filename or invalid preprocessor in script",
                .is_valid = false,
            };
        }
    };
    if(!ags::check_call(builder.BuildModule())) {
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

export class angelscript_system: public script_system<script_lang::angelscript> {
private:
    std::unique_ptr<system_state> m_state;
    angelscript_system_config m_config;
    std::unordered_map<script_id, component_script_info> m_scripts_info;
    std::unordered_map<entity, ags::entity_scripts_runner, entity_hasher, entity_equal> m_script_runners;
    std::unordered_set<entity, entity_hasher, entity_equal> m_marked_commits;

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

    void flush_commits() {
        auto &&context = state().engine.context();
        for(auto en: m_marked_commits) {
            auto &runner = m_script_runners.at(en);
            runner.commit_changes(context, m_scripts_info);
            if(runner.size() == 0) {
                m_script_runners.erase(en);
            }
        }
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
            if(!ags::check_call(
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
            if(!ags::check_call(sbuilder.StartNewModule(state().engine.get(), module_name.c_str()))) {
                return {
                    .error_message = "Unrecoverable error while starting a new module.",
                    .is_valid = false,
                };
            }
            {
                std::string filename = filepath.string();
                const char *c_filename = filename.c_str();
                if(!ags::check_call(sbuilder.AddSectionFromFile(c_filename))) {
                    return {
                        .error_message = "Invalid filename or invalid preprocessor in script",
                        .is_valid = false,
                    };
                }
            };
            if(!ags::check_call(sbuilder.BuildModule())) {
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
                inspect_ags_type(component_derives_ti);
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
                .error_message = "Missing \"" #script_name "\" method", \
                .is_valid = false, \
            }; \
        } \
        saved_info.var_name.acquire(var_name); \
    }

#define ACQUIRE_METHOD_IF_OVERRIDE(var_name, script_name) \
    { \
        auto *(var_name) = component_derives_ti->GetMethodByName(#script_name, false); \
        if((var_name) != nullptr && (var_name)->IsOverride()) { \
            saved_info.var_name = ags::function{var_name}; \
        } \
    }

                    ACQUIRE_METHOD(on_attached, onAttached);
                    ACQUIRE_METHOD(on_detached, onDetached);
                    ACQUIRE_METHOD_IF_OVERRIDE(maybe_update, update);
                    ACQUIRE_METHOD_IF_OVERRIDE(maybe_post_update, postUpdate);
                    ACQUIRE_METHOD_IF_OVERRIDE(maybe_input, input);

#undef ACQUIRE_METHOD
#undef ACQUIRE_METHOD_IF_OVERRIDE
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
    } // namespace st
    [[nodiscard]] scripts_operation_result update_all_scripts(float dt) override {
        flush_commits();
        auto &context = state().engine.context();
        scripts_operation_result result{.is_ok = true};
        for(auto &&[en, runner]: m_script_runners) {
            auto &&this_entity_result = runner.run<ags::lifecycle_method::update>(m_scripts_info, context, dt);
            result.merge(this_entity_result);
        }
        flush_commits();
        return result;
    }
    [[nodiscard]] scripts_operation_result post_update_all_scripts(float dt) override {
        flush_commits();
        auto &context = state().engine.context();
        scripts_operation_result result{.is_ok = true};
        for(auto &&[en, runner]: m_script_runners) {
            auto &&this_entity_result = runner.run<ags::lifecycle_method::post_update>(m_scripts_info, context, dt);
            result.merge(this_entity_result);
        }
        flush_commits();
        return result;
    }
    [[nodiscard]] scripts_operation_result input_all_scripts() override {
        flush_commits();
        auto &context = state().engine.context();
        scripts_operation_result result{.is_ok = true};
        for(auto &&[en, runner]: m_script_runners) {
            auto &&this_entity_result = runner.run<ags::lifecycle_method::input>(m_scripts_info, context);
            result.merge(this_entity_result);
        }
        flush_commits();
        return result;
    }
    [[nodiscard]] scripts_operation_result attach_script(entity en, script_id script_id) override {
        if(!m_script_runners.contains(en)) {
            m_script_runners.try_emplace(en);
        }
        auto &runner = m_script_runners.at(en);
        auto &script_info = m_scripts_info.at(script_id);
        auto &context = state().engine.context();
        auto result = runner.attach_component_type(en, script_id, script_info, context);
        if(result.is_ok) {
            m_marked_commits.insert(en);
        }
        return result;
    }

    [[nodiscard]] scripts_operation_result detach_script(entity en, script_id script_id) override {
        auto &runner = m_script_runners.at(en);
        auto &script_info = m_scripts_info.at(script_id);
        auto &context = state().engine.context();
        auto &&result = runner.detach_component_type(script_id);
        if(result.is_ok) {
            m_marked_commits.insert(en);
        }
        return result;
    }
};
} // namespace st
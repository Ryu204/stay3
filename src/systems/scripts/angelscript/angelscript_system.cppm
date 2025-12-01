module;

#include <cassert>
#include <exception>
#include <format>
#include <memory>
#include <string>
#include <angelscript.h>
#include <scriptbuilder/scriptbuilder.h>
#include <scriptstdstring/scriptstdstring.h>
export module stay3.system.script.angelscript;

import stay3.system.script;
import stay3.ecs;
import stay3.core;
import :engine;

namespace st {

export using ags_script_manager = script_manager<script_lang::angelscript>;
export using ags_scripts = scripts<script_lang::angelscript>;

struct system_state {
    ags_engine engine;
};

export class angelscript_system: public script_system<script_lang::angelscript> {
private:
    std::unique_ptr<system_state> m_state;
    system_state &state() {
        assert(m_state && "Uninitialized or failed initialization");
        return *m_state;
    }
    static bool check_call(int result) {
        return result >= 0;
    }

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
            return {.is_ok = true};
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_ok = false,
            };
        }
    }

    [[nodiscard]] scripts_operation_result shutdown() override {
        m_state.reset();
        return {.is_ok = true};
    }

    [[nodiscard]] script_validation_result load_script(const path &filepath, script_id script_id) override {
        try {
            CScriptBuilder sbuilder;
            if(!check_call(sbuilder.StartNewModule(state().engine.get(), "New script"))) {
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
            return {.is_valid = true, .name = "Not really a name yet"};
        } catch(std::exception &e) {
            return {
                .error_message = e.what(),
                .is_valid = false,
            };
        }
    }
    [[nodiscard]] scripts_operation_result update_all_scripts(float dt) override {
        const auto *mod = state().engine->GetModule("New script");
        assert(mod != nullptr && "Module has not been built (successfully)");
        auto *func = mod->GetFunctionByName("main");
        assert(func != nullptr && "No function named main found");
        auto *ctx = state().engine.context();
        if(!check_call(ctx->Prepare(func))) {
            return {
                .error_message = "Failed to prepare context for function",
                .is_ok = false,
            };
        }
        const auto exec_result = ctx->Execute();
        if(exec_result != asEXECUTION_FINISHED) {
            if(exec_result == asEXECUTION_EXCEPTION) {
                return {
                    .error_message = std::format("Exception occured: {}", ctx->GetExceptionString()),
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
        return {
            .error_message = "Unimplemented",
            .is_ok = false,
        };
    }
    [[nodiscard]] scripts_operation_result detach_script(entity en, script_id script_id) override {
        return {
            .error_message = "Unimplemented",
            .is_ok = false,
        };
    }
};
} // namespace st
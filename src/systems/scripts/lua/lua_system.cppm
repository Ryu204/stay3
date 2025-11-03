module;

#include <type_traits>

export module stay3.system.script.lua;
import stay3.system.script;

export namespace st {
using lua_script_register = script_register<script_lang::lua>;
class lua_script_system: public script_system<script_lang::lua> {
protected:
    [[nodiscard]] script_validation_result load_script(const script_system::path &filepath) override {
        return {.error_message = "Unimplemented", .is_valid = false};
    };
    [[nodiscard]] scripts_operation_result update_all_scripts() override {
        return {.error_message = "Also unimplemented", .is_ok = false};
    }
    [[nodiscard]] scripts_operation_result post_update_all_scripts() override {
        return {.error_message = "Also unimplemented", .is_ok = false};
    };
    [[nodiscard]] scripts_operation_result input_all_scripts() override {
        return {.error_message = "Also unimplemented", .is_ok = false};
    };
};

static_assert(std::is_default_constructible_v<lua_script_system>);
} // namespace st
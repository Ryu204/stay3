module;

#include <optional>
#include <type_traits>

#define SOL_ALL_SAFETIES_ON 1
#include <sol/sol.hpp>

export module stay3.system.script.lua;
import stay3.system.script;
import stay3.node;

export namespace st {

using lua_script_register = script_register<script_lang::lua>;

class lua_script_system: public script_system<script_lang::lua> {
public:
    lua_script_system() = default;
    lua_script_system(const lua_script_system &other): script_system(other) {
        assert(false && "Unimplemented");
    }
    lua_script_system &operator=(const lua_script_system &) {
        assert(false && "Unimplmented");
        return *this;
    }
    lua_script_system(lua_script_system &&) noexcept = delete;
    lua_script_system &operator=(lua_script_system &&) noexcept = delete;
    ~lua_script_system() override = default;

protected:
    [[nodiscard]] scripts_operation_result initialize() override {
        maybe_state = sol::state{};
        return {.is_ok = true};
    }
    [[nodiscard]] scripts_operation_result shutdown() override {
        maybe_state.reset();
        return {.is_ok = true};
    }
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

private:
    std::optional<sol::state> maybe_state{std::nullopt};

    const sol::state &state() {
        return maybe_state.value();
    }
};

static_assert(std::is_default_constructible_v<lua_script_system>);
} // namespace st
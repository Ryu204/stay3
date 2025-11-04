module;

#include <cassert>
#include <optional>
#include <string>

export module stay3.system.script;
import stay3.ecs;
import stay3.node;
import stay3.input;
import :script_database;
import :script_component;
import :error;

export import :script_langs;

namespace st {

export struct script_validation_result {
    std::optional<std::string> error_message{std::nullopt};
    bool is_valid{false};
};

export struct scripts_operation_result {
    std::optional<std::string> error_message{std::nullopt};
    bool is_ok{false};
};

export template<int script_lang>
struct script_register {
    script_component::path path;
};

export template<int script_lang>
class script_system {
public:
    using path = script_component::path;
    void start(tree_context &ctx) {
        setup_signals(ctx.ecs());
        const auto init_result = initialize();
        check_scripts_operations(init_result, sys_type::start);
    }
    void post_update(seconds dt, tree_context &ctx) {
        const auto result = post_update_all_scripts();
        check_scripts_operations(result, sys_type::post_update);
    }
    void update(seconds dt, tree_context &ctx) {
        const auto result = update_all_scripts();
        check_scripts_operations(result, sys_type::update);
    }
    void input(const event &ev, tree_context &ctx) {
        const auto result = input_all_scripts();
        check_scripts_operations(result, sys_type::input);
    }
    void cleanup(tree_context &ctx) {
        const auto result = shutdown();
        check_scripts_operations(result, sys_type::cleanup);
    }

    script_system() = default;
    virtual ~script_system() = default;
    script_system(script_system &&) noexcept = delete;
    script_system &operator=(script_system &&other) noexcept = delete;
    script_system(const script_system &) {
        assert(false && "Cannot copy");
    }
    script_system &operator=(const script_system &) {
        assert(false && "Cannot copy");
    }

protected:
    [[nodiscard]] virtual scripts_operation_result initialize() {
        return {.is_ok = true};
    }
    [[nodiscard]] virtual scripts_operation_result shutdown() {
        return {.is_ok = true};
    }
    [[nodiscard]] virtual script_validation_result load_script(const script_component::path &filepath) = 0;
    [[nodiscard]] virtual scripts_operation_result update_all_scripts() = 0;
    [[nodiscard]] virtual scripts_operation_result post_update_all_scripts() = 0;
    [[nodiscard]] virtual scripts_operation_result input_all_scripts() = 0;

private:
    void setup_signals(ecs_registry &reg) {
        reg
            .on<comp_event::construct, script_register<script_lang>>()
            .template connect<&script_system::on_register_construct>(*this);
        reg
            .on<comp_event::update, script_register<script_lang>>()
            .template connect<&script_system::warn_register_ops_unimplemented>();
    }

    void on_register_construct(ecs_registry &reg, entity en) {
        auto &&[path] = *reg.get<script_register<script_lang>>(en);
        auto &&[error_message, is_valid] = load_script(path);
        if(!is_valid) {
            log::warn("[Script, LANG ID: ", script_lang, "] Failed to load script: ", path.string());
            log::warn("Details: ", error_message ? *error_message : "There are no other diagnostics.");
            return;
        }
        database.register_script(path);
    }

    static void warn_register_ops_unimplemented(ecs_registry &, entity) {
        throw script_error{"Script register component only support construction"};
    }

    static void check_scripts_operations(const scripts_operation_result &res, sys_type ops) {
        if(res.is_ok) { return; }
        log::error("[Script, LANG ID: ", script_lang, "] Failed at operation ", static_cast<int>(ops), " (TODO: Readable ops)");
        log::error("Details: ", res.error_message ? *res.error_message : "There are no other diagnostics.");
    }

    script_database database;
};
} // namespace st
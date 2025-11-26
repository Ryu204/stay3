module;

#include <cassert>
#include <functional>
#include <optional>
#include <string>
#include <variant>

export module stay3.system.script;
import stay3.ecs;
import stay3.node;
import stay3.input;
import :script_database;
import :script_component;
import :error;

export import :script_langs;
export import :script_manager;

namespace st {

export struct script_validation_result {
    std::optional<std::string> error_message{std::nullopt};
    bool is_valid{false};
    std::optional<std::string> name{std::nullopt};
};

export struct scripts_operation_result {
    std::optional<std::string> error_message{std::nullopt};
    bool is_ok{false};
};

export template<script_lang lang>
class script_system {
public:
    using script_id = script_component::id_type;
    using path = script_component::path;
    using script_name = script_component::script_name;

    class script_register {
    public:
        script_register(script_system &system): system{system} {}

        template<std::ranges::range cont>
        void register_scripts(const cont &filepaths) {
            for(auto &&filepath: filepaths) {
                system.get().register_script(filepath);
            }
        };

        template<typename filepath>
        script_id register_script(const filepath &path) {
            return system.get().register_script(path).value();
        }

        [[nodiscard]] script_id id(const script_name &name) const {
            return system.get().database.id_from_name(name);
        }

    private:
        std::reference_wrapper<script_system> system;
    };

    void start(tree_context &ctx) {
        setup_signals(ctx.ecs());
        ctx.vars().emplace<script_register>(*this);
        const auto init_result = initialize();
        check_scripts_operations(init_result, sys_type::start);
    }
    void post_update(seconds dt, tree_context &ctx) {
        const auto result = post_update_all_scripts(dt);
        check_scripts_operations(result, sys_type::post_update);
    }
    void update(seconds dt, tree_context &ctx) {
        const auto result = update_all_scripts(dt);
        check_scripts_operations(result, sys_type::update);
    }
    void input(const event &ev, tree_context &ctx) {
        const auto result = input_all_scripts();
        check_scripts_operations(result, sys_type::input);
    }
    void cleanup(tree_context &ctx) {
        const auto result = shutdown();
        check_scripts_operations(result, sys_type::cleanup);
        ctx.vars().erase<script_register>();
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
    [[nodiscard]] virtual script_validation_result load_script(const path &filepath, script_id script_id) = 0;
    [[nodiscard]] virtual scripts_operation_result update_all_scripts(float dt) = 0;
    [[nodiscard]] virtual scripts_operation_result post_update_all_scripts(float dt) = 0;
    [[nodiscard]] virtual scripts_operation_result input_all_scripts() = 0;
    [[nodiscard]] virtual scripts_operation_result attach_script(entity en, script_id script_id) = 0;
    [[nodiscard]] virtual scripts_operation_result detach_script(entity en, script_id script_id) = 0;

private:
    void setup_signals(ecs_registry &reg) {
        reg
            .on<comp_event::update, script_manager<lang>>()
            .template connect<&script_system::on_script_manager_changed>(*this);
        reg
            .on<comp_event::destroy, script_manager<lang>>()
            .template connect<&script_system::on_script_manager_destroy>(*this);
    }

    std::optional<script_id> register_script(const path &path) {
        const auto maybe_id = database.create_script_id();
        auto &&[error_message, is_valid, name] = load_script(path, maybe_id);
        if(!is_valid) {
            log::warn("[Script, LANG ID: ", script_lang_name(lang), "] Failed to load script: ", path.string());
            log::warn("Details: ", error_message ? *error_message : "There are no other diagnostics.");
            database.delete_unused_script_id(maybe_id);
            return std::nullopt;
        }
        assert(name.has_value() && "Valid script must have a name");
        database.register_script(maybe_id, path, name.value());
        log::info("[Script, LANG ID: ", script_lang_name(lang), "] Loaded \"", name.value(), "\" from ", path.string());
        return maybe_id;
    }

    static void check_scripts_operations(const scripts_operation_result &res, sys_type ops) {
        if(res.is_ok) { return; }
        log::error("[Script, LANG ID: ", script_lang_name(lang), "] Failed at operation: \"", sys_type_name(ops), '"');
        log::error("Details: ", res.error_message ? *res.error_message : "There are no other diagnostics.");
    }

    void on_script_manager_changed(ecs_registry &reg, entity en) {
        auto manager = reg.get<script_manager<lang>>(en);
        if(!manager->has_unsaved_changes()) { return; }
        const visit_helper visitor{
            [en, this](const script_manager<lang>::add &ch) {
                const auto res = attach_script(en, ch.id);
                if(!res.is_ok) {
                    log::error("[Script, LANG ID: ", script_lang_name(lang), "] Errors adding script component: ", res.error_message ? *res.error_message : "There are no other diagnostics.");
                }
            },
            [en, this](const script_manager<lang>::remove &ch) {
                const auto res = detach_script(en, ch.id);
                if(!res.is_ok) {
                    log::error("[Script, LANG ID: ", script_lang_name(lang), "] Errors removing script component: ", res.error_message ? *res.error_message : "There are no other diagnostics.");
                }
            }};
        for(auto &&change: manager->unsaved_changes()) {
            std::visit(visitor, change);
        }
        manager->save();
    }

    void on_script_manager_destroy(ecs_registry &reg, entity en) {
        auto manager = reg.get<script_manager<lang>>(en);
        assert(!manager->has_unsaved_changes() && "On update handler should have been called and flushed the changes");
        for(const auto &id: manager->current_scripts()) {
            const auto res = detach_script(en, id);
            if(!res.is_ok) {
                log::error("[Script, LANG ID: ", script_lang_name(lang), "] Errors removing script component: ", res.error_message ? *res.error_message : "There are no other diagnostics.");
            }
        }
    }

    script_database database;
};

export template<script_lang lang>
using scripts = script_system<lang>::script_register;

export using script_id = script_component::id_type;

} // namespace st
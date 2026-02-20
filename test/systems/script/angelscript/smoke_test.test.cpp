#include <catch2/catch_all.hpp>
import stay3;
using namespace st;

struct scripts_name {
    static inline st::script_id bird{};
};

struct sys {
    static void start(tree_context &ctx) {
        add_scripts(ctx);
        auto &ecs = ctx.ecs();
        auto en = ctx.root().entities().create();
        ecs.emplace<mut<ags_script_manager>>(en)->add_scripts(scripts_name::bird);
    }

    static void add_scripts(tree_context &ctx) {
        auto &&scripts = ctx.vars().get<ags_scripts>();
        scripts_name::bird = scripts.register_script("./assets/scripts/bird.as");
    }

    static sys_run_result post_update(float, tree_context &ctx) {
        const auto &scripts = ctx.vars().get<ags_scripts>();
        REQUIRE_FALSE(scripts.was_error_occured());
        return sys_run_result::exit;
    }
};

TEST_CASE("Smoke test") {
    app_launcher app{};
    app.systems().add<sys>().run_as<sys_type::start>(sys_priority::lowest).run_as<sys_type::post_update>();
    REQUIRE_NOTHROW(app.launch());
}
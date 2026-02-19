#include <catch2/catch_all.hpp>
#include "catch2/catch_test_macros.hpp"
import stay3;

using namespace st;

const char *const ctor_test_script = R"(
external shared abstract class Component;

class VectorConstructorTest : Component {
    protected void start() override {
        Vec2i a(1, 3);
        assert(a.x == 1 && a.y == 3, "Full components");
        Vec2i b(3);
        assert(a.x == 3 && a.y == 0.0, "Partial components");
        Vec2i c;
        assert(c.x == 0 && c.y == 0.F, "Default components");
    }
}
)";

TEST_CASE("Constructors") {
    struct system {
        void start(tree_context &ctx) {
            auto &ecs = ctx.ecs();
            auto &scripts = ctx.vars().get<ags_scripts>();
            const auto ctor_script_id = scripts.register_script_from_memory("Constructor test", ctor_test_script);

            auto &node = ctx.root().add_child();
            const auto en = node.entities().create();
            ecs.emplace<mut<ags_script_manager>>(en)->add_scripts(ctor_script_id);
        }
    };

    app_launcher app;
    app.systems().add<system>().run_as<sys_type::start>(sys_priority::lowest);
    REQUIRE_NOTHROW(app.launch());
}
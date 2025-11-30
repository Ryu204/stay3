#include <iostream>
import stay3;
using namespace st;

namespace scripts_name {
st::script_id bird{};
}

struct sys {
    static void start(tree_context &ctx) {
        add_scripts(ctx);
        auto &cam_node = ctx.root().add_child();
        auto cam_en = cam_node.entities().create();
        auto &ecs = ctx.ecs();
        ecs.emplace<main_camera>(cam_en);
        ecs.emplace<camera>(cam_en, camera{});
        ecs.emplace<mut<ags_script_manager>>(cam_en)->add_scripts(scripts_name::bird);
    }

    static void add_scripts(tree_context &ctx) {
        auto &&scripts = ctx.vars().get<ags_scripts>();
        scripts_name::bird = scripts.register_script("./assets/scripts/bird.as");
    }
};

int main() {
    try {
        app_launcher app{};
        app.systems().add<sys>().run_as<sys_type::start>(sys_priority::lowest);
        app.launch();
    } catch(std::exception &e) {
        std::cerr << e.what() << '\n';
    } catch(...) {
        std::cerr << "Unknown error\n";
    }
}
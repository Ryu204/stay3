#include <iostream>
import stay3;

struct sys {
    static void start(st::tree_context &ctx) {
        auto &cam_node = ctx.root().add_child();
        auto cam_en = cam_node.entities().create();
        auto &ecs = ctx.ecs();
        ecs.emplace<st::main_camera>(cam_en);
        ecs.emplace<st::camera>(cam_en, st::camera{});
    }
};

int main() {
    try {
        st::app_launcher app{};
        app.systems().add<sys>().run_as<st::sys_type::start>();
        app.launch();
    } catch(std::exception &e) {
        std::cerr << e.what() << '\n';
    } catch(...) {
        std::cerr << "Unknown error\n";
    }
}
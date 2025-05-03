#include <exception>
#include <iostream>
import stay3;

using namespace st;

struct show_atlas {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &root = ctx.root();

        const auto en = root.entities().create();
        auto ft = reg.emplace<font>(en, "./assets/fonts/Roboto-Regular.ttf");

        auto txt = reg.emplace<text>(en, text{.content = u8"Hello", .font = en});

        auto camera_en = root.entities().create();
        reg.emplace<camera>(camera_en, camera{.data = camera::orthographic_data{.width = 10}});
        reg.emplace<main_camera>(camera_en);
    }
};

int main() try {
    app my_app;
    my_app.systems().add<show_atlas>().run_as<sys_type::start>(sys_priority::very_low);
    my_app.run();
} catch(std::exception &e) {
    std::cerr << e.what() << '\n';
} catch(...) {
    std::cerr << "Unknown error\n";
}
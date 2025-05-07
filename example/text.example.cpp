#include <exception>
#include <iostream>
import stay3;

using namespace st;

struct show_atlas {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto &root = ctx.root();

        const auto en1 = root.entities().create();
        const auto en2 = root.entities().create();
        reg.emplace<font>(en1, "./assets/fonts/Roboto-Regular.ttf");
        reg.emplace<font>(en2, "./assets/fonts/Tagesschrift-Regular.ttf");

        auto txt = reg.emplace<text>(en1, text{.content = u8"the quick brown dog jump over the lazy fox", .font = en1});
        reg.emplace<text>(en2, text{.content = u8"BRAVO\tT. W. O", .size = 64, .font = en2});
        reg.emplace<mut<transform>>(en2)->translate(vec_down + vec_forward);

        auto camera_en = root.entities().create();
        reg.emplace<camera>(camera_en, camera{.data = camera::orthographic_data{}});
        reg.emplace<main_camera>(camera_en);
        reg.get<mut<transform>>(camera_en)->translate(vec_back * 5.F);
    }

    static sys_run_result input(const event &ev, tree_context &ctx) {
        auto &reg = ctx.ecs();
        if(const auto *key_pressed = ev.try_get<event::key_pressed>(); key_pressed != nullptr) {
            if(key_pressed->code == scancode::esc) {
                return sys_run_result::exit;
            }
            if(key_pressed->code == scancode::space) {
                for(auto en: reg.view<text>()) {
                    reg.get<mut<transform>>(en)->rotate(vec_back, 1.F);
                }
            }
        }
        return sys_run_result::noop;
    }
};

int main() try {
    app my_app;
    my_app
        .systems()
        .add<show_atlas>()
        .run_as<sys_type::start>(sys_priority::very_low);
    my_app
        .systems()
        .add<show_atlas>()
        .run_as<sys_type::input>();
    my_app.run();
} catch(std::exception &e) {
    std::cerr << e.what() << '\n';
} catch(...) {
    std::cerr << "Unknown error\n";
}
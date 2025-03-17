#include <exception>
#include <iostream>
import stay3;

using namespace st;

struct setup_system {
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto entity = ctx.root().entities().create();
        reg.add_component<const mesh_data>(entity, mesh_plane(vec2f{1.F, 1.F}, vec4f{0.05F, 0.6F, 0.8F, 1.F}));
    }
};

int main() {
    try {
        app my_app{
            {
                .window = {
                    .size = {600u, 400u},
                    .name = "My cool window",
                },
                .updates_per_second = 20,
                .render = {.power_pref = render_config::power_preference::low},
            },
        };
        my_app.enable_default_systems();
        my_app.systems().add<setup_system>().run_as<sys_type::start>();
        my_app.run();
    } catch(std::exception &e) {
        std::cerr << "Exception happened: " << e.what() << '\n';
    }
}

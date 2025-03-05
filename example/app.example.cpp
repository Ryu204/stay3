#include <iostream>
import stay3;

using namespace st;

struct my_system {
    static void start(tree_context &ctx) {
        std::cout << "Init scene\n";
        auto &child1 = ctx.root().add_child();
        ctx.ecs().add_component<int>(child1.entities().create(), 1);
        ctx.ecs().add_component<int>(child1.entities().create(), 3);

        auto &child2 = ctx.root().add_child();
        ctx.ecs().add_component<int>(child2.entities().create(), 4);
    }

    static void update(seconds, tree_context &ctx) {
        auto sum = 0;
        for(auto &&[en, val]: ctx.ecs().each<const int>()) {
            std::cout << "[node " << ctx.get_node(en).id() << ", value " << val << "];";
            sum += val;
        }
        std::cout << "\nSum = " << sum << '\n';
    }

    static void render(tree_context &) {
        std::cout << "Rendered\n";
    }

    static void cleanup(tree_context &) {
        std::cout << "Cleanup\n";
    }
};

int main() {
    app my_app;
    my_app.systems()
        .add<my_system>()
        .run_as<sys_type::start>()
        .run_as<sys_type::update>()
        .run_as<sys_type::render>()
        .run_as<sys_type::cleanup>();
    my_app.run();
}

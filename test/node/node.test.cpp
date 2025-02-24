#include <catch2/catch_all.hpp>

import stay3.node;

TEST_CASE("node basic functionality") {
    st::node_registry registry;
    auto root = st::node::create_root(registry);

    SECTION("Root node is correctly registered") {
        REQUIRE(root->id() == registry.get_root().id());
    }

    SECTION("Adding child nodes") {
        auto &child1 = root->add_child();
        auto &child2 = root->add_child();
        REQUIRE(child1.id() != child2.id());
    }

    SECTION("Parent-child relationship is maintained") {
        auto &child = root->add_child();
        REQUIRE(&child.parent() == root.get());
    }

    SECTION("Reparenting nodes") {
        auto &child1 = root->add_child();
        auto &child2 = root->add_child();
        child1.reparent(child2);
        REQUIRE(&child1.parent() == &child2);
    }

    SECTION("Ancestor relationship") {
        auto &child1 = root->add_child();
        auto &child2 = root->add_child();
        auto &child3 = child1.add_child();

        REQUIRE(root->is_ancestor_of(child3));
        REQUIRE(child1.is_ancestor_of(child3));
        REQUIRE_FALSE(child2.is_ancestor_of(child3));
        REQUIRE_FALSE(child3.is_ancestor_of(child3));
    }
}

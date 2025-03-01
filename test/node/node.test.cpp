#include <catch2/catch_all.hpp>

import stay3.node;

TEST_CASE("node basic functionality") {
    st::tree_context context;
    auto &root = context.root();

    SECTION("Root node is correctly registered") {
        REQUIRE(root.id() == context.root().id());
        REQUIRE((&context.get_node(root.id())) == (&root));
    }

    SECTION("Adding child nodes") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        REQUIRE(child1.id() != child2.id());
        REQUIRE((&context.get_node(child1.id())) == (&child1));
    }

    SECTION("Parent-child relationship is maintained") {
        auto &child = root.add_child();
        REQUIRE(&child.parent() == &root);
    }

    SECTION("Reparenting nodes") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        child1.reparent(child2);
        REQUIRE(&child1.parent() == &child2);
    }

    SECTION("Ancestor relationship") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        auto &child3 = child1.add_child();

        REQUIRE(root.is_ancestor_of(child3));
        REQUIRE(child1.is_ancestor_of(child3));
        REQUIRE_FALSE(child2.is_ancestor_of(child3));
        REQUIRE_FALSE(child3.is_ancestor_of(child3));
    }
}

#include <catch2/catch_all.hpp>

import stay3.node;
import stay3.ecs;

using namespace st;

TEST_CASE("Entity to node mapping") {
    tree_context context;
    auto &root = context.root();

    SECTION("Entity should be added to mapping") {
        auto en = root.entities().create();
        REQUIRE(context.ecs().contains_entity(en));
        REQUIRE((&context.get_node(en)) == &root);
    }

    SECTION("Children's entities should be added to mapping") {
        auto &child1 = root.add_child();
        auto en = child1.entities().create();
        REQUIRE(context.ecs().contains_entity(en));
        REQUIRE((&context.get_node(en)) == (&child1));
    }

    SECTION("Deleted entity is removed from ecs registry") {
        auto en = root.entities().create();
        root.entities().destroy(0);
        REQUIRE_FALSE(context.ecs().contains_entity(en));
    }
}

struct event_listener {
    node *node{};
    entity en;
    void on_event(class node &node, entity en) {
        this->node = &node;
        this->en = en;
    }
};

TEST_CASE("Entities event") {
    tree_context context;
    auto &root = context.root();
    event_listener lis;

    SECTION("Create event") {
        root.on_entity_created().connect<&event_listener::on_event>(lis);
        auto en = root.entities().create();
        REQUIRE(lis.en == en);
        REQUIRE(lis.node == &root);

        auto &child = root.add_child();
        child.on_entity_created().connect<&event_listener::on_event>(lis);
        en = child.entities().create();
        REQUIRE(lis.en == en);
        REQUIRE(lis.node == (&child));

        child.on_entity_created().disconnect<&event_listener::on_event>(lis);
        en = child.entities().create();
        REQUIRE(lis.en != en);
        REQUIRE(lis.node == (&child));
    }

    SECTION("Destroy event") {
        root.on_entity_destroyed().connect<&event_listener::on_event>(lis);
        auto en = root.entities().create();
        root.entities().destroy(0);
        REQUIRE(lis.en == en);
        REQUIRE(lis.node == &root);

        auto &child = root.add_child();
        child.on_entity_destroyed().connect<&event_listener::on_event>(lis);
        en = child.entities().create();
        child.entities().destroy(0);
        REQUIRE(lis.en == en);
        REQUIRE(lis.node == (&child));

        child.on_entity_destroyed().disconnect<&event_listener::on_event>(lis);
        en = child.entities().create();
        child.entities().destroy(0);
        REQUIRE(lis.en != en);
        REQUIRE(lis.node == (&child));
    }
}
#include <catch2/catch_all.hpp>

import stay3.ecs;

using namespace st;

struct test_component {
    int value;
};

TEST_CASE("component_ref constructor") {
    entity null_entity{};
    component_ref<test_component> ref_default{};
    component_ref<test_component> ref_null{null_entity};

    REQUIRE(ref_default.is_null());
    REQUIRE(ref_null.is_null());

    ecs_registry registry{};
    auto entity = registry.create();
    component_ref<test_component> ref_entity{entity};

    REQUIRE(ref_entity.entity() == entity);
    REQUIRE_FALSE(ref_entity.is_null());
}

TEST_CASE("assignment") {
    ecs_registry registry{};
    auto entity = registry.create();
    registry.emplace<test_component>(entity, 42);

    component_ref<test_component> ref{};
    ref = entity;

    REQUIRE(ref.entity() == entity);
}

TEST_CASE("retrieval") {
    ecs_registry registry{};
    auto entity = registry.create();
    registry.emplace<test_component>(entity, 42);

    component_ref<test_component> ref{entity};
    REQUIRE(ref.get(registry)->value == 42);
}

TEST_CASE("mutation") {
    ecs_registry registry{};
    auto entity = registry.create();
    registry.emplace<test_component>(entity, 10);

    component_ref<test_component> ref{entity};
    auto mut_component = ref.get_mut(registry);
    mut_component->value = 99;

    REQUIRE(registry.get<test_component>(entity)->value == 99);
}

TEST_CASE("comparison") {
    ecs_registry registry{};
    auto entity1 = registry.create();
    auto entity2 = registry.create();

    component_ref<test_component> ref1{entity1};
    component_ref<test_component> ref2{entity2};
    component_ref<test_component> ref3{entity1};

    REQUIRE(ref1 == ref3);
    REQUIRE(ref1 != ref2);
}

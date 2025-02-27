#include <catch2/catch_all.hpp>

import stay3.ecs;

struct dummy {
    int value;
};

TEST_CASE("ecs_registry: Entity and Component Management", "[ecs_registry]") {
    st::ecs_registry registry;

    SECTION("Entity creation and destruction") {
        auto en = registry.create_entity();
        REQUIRE(registry.contains_entity(en));

        registry.destroy_entity(en);
        REQUIRE_FALSE(registry.contains_entity(en));
    }

    SECTION("Component operations") {
        auto en = registry.create_entity();

        REQUIRE_FALSE(registry.has_components<dummy>(en));

        SECTION("Emplace, get, and modify component") {
            registry.emplace_component<dummy>(en, 42);
            REQUIRE(registry.has_components<dummy>(en));

            auto &comp = registry.get_component<dummy>(en);
            REQUIRE(comp.value == 42);

            SECTION("Modify via get_component") {
                comp.value = 100;
                auto &comp2 = registry.get_component<dummy>(en);
                REQUIRE(comp2.value == 100);
            }
        }

        SECTION("Remove component") {
            registry.emplace_component<dummy>(en, 10);
            REQUIRE(registry.has_components<dummy>(en));
            registry.remove_component<dummy>(en);
            REQUIRE_FALSE(registry.has_components<dummy>(en));
        }

        SECTION("Patch component") {
            registry.emplace_component<dummy>(en, 20);
            registry.patch_component<dummy>(en, [](dummy &comp) {
                comp.value = 84;
            });
            auto &comp = registry.get_component<dummy>(en);
            REQUIRE(comp.value == 84);
        }

        SECTION("Replace component") {
            registry.emplace_component<dummy>(en, 30);
            registry.replace_component<dummy>(en, 150);
            auto &comp = registry.get_component<dummy>(en);
            REQUIRE(comp.value == 150);
        }
    }
}

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

    SECTION("Single component operations") {
        auto en = registry.create_entity();

        REQUIRE_FALSE(registry.has_components<dummy>(en));

        SECTION("Emplace, get, and modify component") {
            registry.add_component<dummy>(en, 42);
            REQUIRE(registry.has_components<dummy>(en));

            auto &&comp = registry.get_component<dummy>(en);
            REQUIRE(comp->value == 42);

            SECTION("Modify via get_component") {
                comp->value = 100;
                auto &&comp2 = registry.get_component<const dummy>(en);
                REQUIRE(comp2.value == 100);
            }
        }

        SECTION("Remove component") {
            registry.add_component<dummy>(en, 10);
            REQUIRE(registry.has_components<dummy>(en));
            registry.remove_component<dummy>(en);
            REQUIRE_FALSE(registry.has_components<dummy>(en));
        }

        SECTION("Patch component") {
            registry.add_component<dummy>(en, 20);
            registry.patch_component<dummy>(en, [](dummy &comp) {
                comp.value = 84;
            });
            auto &&comp = registry.get_component<const dummy>(en);
            REQUIRE(comp.value == 84);
        }

        SECTION("Replace component") {
            registry.add_component<dummy>(en, 30);
            registry.replace_component<dummy>(en, 150);
            auto &&comp = registry.get_component<const dummy>(en);
            REQUIRE(comp.value == 150);
        }
    }

    SECTION("Iteration over components") {
        auto en1 = registry.create_entity();
        auto en2 = registry.create_entity();
        auto en3 = registry.create_entity();

        registry.add_component<dummy>(en1, 10);
        registry.add_component<dummy>(en2, 20);
        registry.add_component<dummy>(en3, 30);

        struct second_comp {
            float value;
            second_comp(float value)
                : value{value} {}
        };

        registry.add_component<second_comp>(en1, 1.0f);
        registry.add_component<second_comp>(en3, 3.0f);

        SECTION("Single component iteration") {
            int count = 0;
            int sum = 0;

            for(auto &&[entity, comp]: registry.each<dummy>()) {
                count++;
                sum += comp.value;
            }

            REQUIRE(count == 3);
            REQUIRE(sum == 60);
        }

        SECTION("Multiple component iteration") {
            int count = 0;

            for(auto &&[entity, d, s]: registry.each<dummy, second_comp>()) {
                count++;
                REQUIRE(((d.value == 10 && s.value == 1.0f) || (d.value == 30 && s.value == 3.0f)));

                d.value *= 2;
                s.value *= 10.0f;
            }

            REQUIRE(count == 2);

            auto &&d1 = registry.get_component<const dummy>(en1);
            auto &&s1 = registry.get_component<const second_comp>(en1);
            REQUIRE(d1.value == 20);
            REQUIRE(s1.value == 10.0f);

            auto &&d3 = registry.get_component<const dummy>(en3);
            auto &&s3 = registry.get_component<const second_comp>(en3);
            REQUIRE(d3.value == 60);
            REQUIRE(s3.value == 30.0f);
        }
    }
}

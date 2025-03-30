#include <iterator>
#include <type_traits>
#include <catch2/catch_all.hpp>

import stay3.ecs;
using Catch::Approx;

struct dummy {
    int value;
};

struct empty_dummy {
};

struct entity_destroyed_handler {
    void on_event() {
        signaled = true;
    }
    bool signaled{false};
};

TEST_CASE("Concepts are satisfied") {
    using each_t = std::decay_t<decltype(std::declval<st::ecs_registry>().each<dummy, empty_dummy>())>;
    STATIC_REQUIRE(std::input_iterator<each_t::iterator>);
    STATIC_REQUIRE(std::ranges::range<each_t>);
}

TEST_CASE("Entity and Component Management") {
    st::ecs_registry registry;

    SECTION("Entity null check") {
        st::entity en;
        REQUIRE(en.is_null());
        en = registry.create_entity();
        REQUIRE_FALSE(en.is_null());
    }

    SECTION("Entity creation and destruction") {
        auto en = registry.create_entity();
        REQUIRE(registry.contains_entity(en));

        registry.destroy_entity(en);
        REQUIRE_FALSE(registry.contains_entity(en));

        SECTION("On entity destroyed signal") {
            entity_destroyed_handler handler;
            registry.on_entity_destroyed().connect<&entity_destroyed_handler::on_event>(handler);
            REQUIRE_FALSE(handler.signaled);
            registry.destroy_entity(registry.create_entity());
            REQUIRE(handler.signaled);
        }
    }

    SECTION("Single component operations") {
        auto en = registry.create_entity();

        SECTION("Add and has component check") {
            REQUIRE_FALSE(registry.has_components<dummy>(en));
            registry.add_component<dummy>(en, 42);
            REQUIRE(registry.has_components<dummy>(en));
            SECTION("Empty component") {
                REQUIRE_FALSE(registry.has_components<empty_dummy>(en));
                registry.add_component<empty_dummy>(en);
                REQUIRE(registry.has_components<empty_dummy>(en));
                REQUIRE(registry.has_components<dummy, empty_dummy>(en));
            }
        }

        SECTION("Get component") {
            registry.add_component<const dummy>(en, 42);
            registry.add_component<empty_dummy>(en);
            SECTION("Read proxy") {
                auto comp = registry.get_components<const dummy>(en);
                REQUIRE(comp->value == 42);
            }
            SECTION("Write proxy") {
                auto comp = registry.get_components<dummy>(en);
                REQUIRE(comp->value == 42);
                comp->value = 50;
                REQUIRE(registry.get_components<const dummy>(en)->value == 50);
            }
            SECTION("Empty proxy") {
                REQUIRE_NOTHROW(registry.get_components<empty_dummy>(en));
                STATIC_REQUIRE(std::is_empty_v<std::decay_t<decltype(registry.get_components<empty_dummy>(en))>>);
            }
            SECTION("Multiple components") {
                auto [comp, ecomp] = registry.get_components<dummy, empty_dummy>(en);
                REQUIRE(comp->value == 42);
            }
        }

        SECTION("Remove component") {
            registry.add_component<dummy>(en, 10);
            registry.remove_component<dummy>(en);
            REQUIRE_FALSE(registry.has_components<dummy>(en));
        }

        SECTION("Patch component") {
            registry.add_component<dummy>(en, 20);
            registry.patch_component<dummy>(en, [](dummy &comp) {
                comp.value = 84;
            });
            auto comp = registry.get_components<const dummy>(en);
            REQUIRE(comp->value == 84);
        }

        SECTION("Replace component") {
            registry.add_component<dummy>(en, 30);
            registry.replace_component<dummy>(en, 150);
            auto comp = registry.get_components<const dummy>(en);
            REQUIRE(comp->value == 150);
        }

        SECTION("Clear component") {
            registry.add_component<dummy>(en, 30);
            registry.add_component<empty_dummy>(en);
            registry.clear_component<dummy>();
            REQUIRE_FALSE(registry.has_components<dummy>(en));
        }
    }
}

TEST_CASE("Sort and iteration") {
    st::ecs_registry registry;
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

    SECTION("Iteration over components") {
        SECTION("Single component iteration") {
            int count = 0;
            int sum = 0;
            for(auto &&[entity, comp]: registry.each<const dummy>()) {
                count++;
                sum += comp->value;
            }
            REQUIRE(count == 3);
            REQUIRE(sum == 60);
        }

        SECTION("Multiple component iteration") {
            int count = 0;

            for(auto &&[entity, d, s]: registry.each<dummy, second_comp>()) {
                count++;
                REQUIRE(((d->value == 10 && s->value == 1.0f) || (d->value == 30 && s->value == 3.0f)));

                d->value *= 2;
                s->value *= 10.0f;
            }

            REQUIRE(count == 2);

            auto d1 = registry.get_components<const dummy>(en1);
            auto s1 = registry.get_components<const second_comp>(en1);
            REQUIRE(d1->value == 20);
            REQUIRE(s1->value == 10.0f);

            auto d3 = registry.get_components<const dummy>(en3);
            auto s3 = registry.get_components<const second_comp>(en3);
            REQUIRE(d3->value == 60);
            REQUIRE(s3->value == 30.0f);
        }
    }

    SECTION("Sort") {
        SECTION("Sort") {
            registry.sort<dummy>([](const dummy &lhs, const dummy &rhs) {
                return lhs.value > rhs.value;
            });

            std::vector<int> values;
            for(auto &&[entity, comp]: registry.each<const dummy>()) {
                values.push_back(comp->value);
            }

            REQUIRE(std::ranges::is_sorted(values, std::greater<int>{}));
        }
        SECTION("Sort by entity") {
            registry.sort<dummy>([&registry](const st::entity &lhs, const st::entity &rhs) {
                return registry.get_components<const dummy>(lhs)->value > registry.get_components<const dummy>(rhs)->value;
            });

            std::vector<int> values;
            for(auto &&[entity, comp]: registry.each<const dummy>()) {
                values.push_back(comp->value);
            }

            REQUIRE(std::ranges::is_sorted(values, std::greater<int>{}));
        }
    }
}

TEST_CASE("Context Management") {
    st::ecs_registry registry;
    struct audio_context {
        float volume{0.8f};
    };
    struct physics_context {
        float gravity;
        bool enable_collision;
    };

    SECTION("Add context") {
        REQUIRE_NOTHROW(registry.add_context<physics_context>(3.711F, true));

        const auto &physics = registry.get_context<const physics_context>();
        REQUIRE(physics.gravity == Approx(3.711f));
        REQUIRE(physics.enable_collision == true);

        SECTION("Multiple contexts") {
            registry.add_context<audio_context>();
            REQUIRE(registry.get_context<audio_context>().volume == Approx(0.8f));
        }
    }

    SECTION("Modify context") {
        registry.add_context<physics_context>(3.711F, true);
        auto &phys = registry.get_context<physics_context>();
        phys.enable_collision = false;
        REQUIRE_FALSE(registry.get_context<const physics_context>().enable_collision);
    }

    SECTION("Modify on creation") {
        auto &phys = registry.add_context<physics_context>(3.123F, false);
        REQUIRE_FALSE(registry.get_context<const physics_context>().enable_collision);
        phys.enable_collision = true;
        REQUIRE(registry.get_context<const physics_context>().enable_collision);
    }
}
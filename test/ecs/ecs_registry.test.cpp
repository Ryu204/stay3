#include <iterator>
#include <type_traits>
#include <catch2/catch_all.hpp>

import stay3.ecs;
using Catch::Approx;
using Catch::Matchers::UnorderedRangeEquals;

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

TEST_CASE("Traits") {
    SECTION("Concepts are satisfied") {
        using each_t = std::decay_t<decltype(std::declval<st::ecs_registry>().each<dummy, empty_dummy>(st::exclude<float, int>))>;
        STATIC_REQUIRE(std::input_iterator<each_t::iterator>);
        STATIC_REQUIRE(std::ranges::range<each_t>);

        using view_t = std::decay_t<decltype(std::declval<st::ecs_registry>().view<empty_dummy>())>;
        STATIC_REQUIRE(std::input_iterator<view_t::iterator>);
        STATIC_REQUIRE(std::ranges::range<view_t>);
    }
    SECTION("Type traits work correctly") {
        STATIC_REQUIRE(st::is_mut_v<st::mut<dummy>>);
        STATIC_REQUIRE(st::is_mut_v<st::mut<empty_dummy>>);
        STATIC_REQUIRE_FALSE(st::is_mut_v<dummy>);
        STATIC_REQUIRE_FALSE(st::is_mut_v<empty_dummy>);

        STATIC_REQUIRE(std::is_same_v<st::remove_mut_t<st::mut<dummy>>, dummy>);
        STATIC_REQUIRE(std::is_same_v<st::remove_mut_t<st::mut<empty_dummy>>, empty_dummy>);
        STATIC_REQUIRE(std::is_same_v<st::remove_mut_t<dummy>, dummy>);
        STATIC_REQUIRE(std::is_same_v<st::remove_mut_t<empty_dummy>, empty_dummy>);

        STATIC_REQUIRE(std::is_same_v<std::decay_t<decltype(st::exclude<dummy, empty_dummy>)>, st::exclude_t<dummy, empty_dummy>>);
    }
}

TEST_CASE("Entity and Component Management") {
    st::ecs_registry registry;

    SECTION("Entity null check") {
        st::entity en;
        REQUIRE(en.is_null());
        en = registry.create();
        REQUIRE_FALSE(en.is_null());
    }

    SECTION("Entity creation and destruction") {
        auto en = registry.create();
        REQUIRE(registry.contains(en));

        registry.destroy(en);
        REQUIRE_FALSE(registry.contains(en));

        SECTION("destroy if exist") {
            REQUIRE_NOTHROW(registry.destroy_if_exist(en));
            auto en2 = registry.create();
            registry.destroy_if_exist(en2);
            REQUIRE_FALSE(registry.contains(en2));
        }

        SECTION("On entity destroyed signal") {
            entity_destroyed_handler handler;
            registry.on_entity_destroyed().connect<&entity_destroyed_handler::on_event>(handler);
            REQUIRE_FALSE(handler.signaled);

            SECTION("Destroy") {
                registry.destroy(registry.create());
                REQUIRE(handler.signaled);
            }
            SECTION("Destroy if exist") {
                registry.destroy_if_exist(registry.create());
                REQUIRE(handler.signaled);
            }
        }
    }

    SECTION("Single component operations") {
        auto en = registry.create();

        SECTION("Add and has component check") {
            REQUIRE_FALSE(registry.contains<dummy>(en));
            registry.emplace<dummy>(en, 42);
            REQUIRE(registry.contains<dummy>(en));

            SECTION("Empty component") {
                REQUIRE_FALSE(registry.contains<empty_dummy>(en));
                registry.emplace<empty_dummy>(en);
                REQUIRE(registry.contains<empty_dummy>(en));
                REQUIRE(registry.contains<dummy, empty_dummy>(en));
            }

            SECTION("Conditional emplaces") {
                registry.emplace_or_replace<dummy>(en, 100);
                REQUIRE(registry.get<dummy>(en)->value == 100);
                auto dum = registry.emplace_if_not_exist<dummy>(en, 200);
                REQUIRE(dum->value == 100);

                SECTION("Emplace with no prior component") {
                    auto en2 = registry.create();
                    REQUIRE(registry.emplace_or_replace<dummy>(en2, 42)->value == 42);
                    auto en3 = registry.create();
                    REQUIRE(registry.emplace_if_not_exist<dummy>(en3, 45)->value == 45);
                }
            }
        }

        SECTION("Get component") {
            registry.emplace<dummy>(en, 42);
            registry.emplace<empty_dummy>(en);
            SECTION("Read proxy") {
                auto comp = registry.get<dummy>(en);
                REQUIRE(comp->value == 42);
            }
            SECTION("Write proxy") {
                auto comp = registry.get<st::mut<dummy>>(en);
                REQUIRE(comp->value == 42);
                comp->value = 50;
                REQUIRE(registry.get<dummy>(en)->value == 50);
            }
            SECTION("Empty proxy") {
                REQUIRE_NOTHROW(registry.get<empty_dummy>(en));
                STATIC_REQUIRE(std::is_empty_v<std::decay_t<decltype(registry.get<empty_dummy>(en))>>);
            }
            SECTION("Multiple components") {
                auto [comp, ecomp] = registry.get<st::mut<dummy>, empty_dummy>(en);
                REQUIRE(comp->value == 42);
            }
        }

        SECTION("Remove component") {
            SECTION("Single") {
                registry.emplace<dummy>(en, 10);
                registry.destroy<dummy>(en);
                REQUIRE_FALSE(registry.contains<dummy>(en));
            }
            SECTION("Multiple") {
                registry.emplace<dummy>(en, 10);
                registry.emplace<float>(en, 10.F);
                registry.destroy<dummy, float>(en);
                REQUIRE_FALSE(registry.contains<dummy>(en));
                REQUIRE_FALSE(registry.contains<float>(en));
            }
            SECTION("Conditional remove") {
                registry.emplace<dummy>(en);
                registry.destroy_if_exist<dummy>(en);
                REQUIRE_FALSE(registry.contains<dummy>(en));
                registry.destroy_if_exist<dummy>(en);
                REQUIRE_FALSE(registry.contains<dummy>(en));
            }
        }

        SECTION("Patch component") {
            registry.emplace<dummy>(en, 20);
            registry.patch<dummy>(en, [](dummy &comp) {
                comp.value = 84;
            });
            auto comp = registry.get<dummy>(en);
            REQUIRE(comp->value == 84);
        }

        SECTION("Replace component") {
            registry.emplace<dummy>(en, 30);
            registry.replace<dummy>(en, 150);
            auto comp = registry.get<dummy>(en);
            REQUIRE(comp->value == 150);
        }

        SECTION("Clear component") {
            registry.emplace<dummy>(en, 30);
            registry.emplace<empty_dummy>(en);
            registry.destroy_all<dummy>();
            REQUIRE_FALSE(registry.contains<dummy>(en));
        }
    }
}

TEST_CASE("Sort and iteration") {
    st::ecs_registry registry;
    auto en1 = registry.create();
    auto en2 = registry.create();
    auto en3 = registry.create();

    registry.emplace<dummy>(en1, 10);
    registry.emplace<dummy>(en2, 20);
    registry.emplace<dummy>(en3, 30);

    struct second_comp {
        float value;
        second_comp(float value)
            : value{value} {}
    };

    registry.emplace<second_comp>(en1, 1.0f);
    registry.emplace<second_comp>(en3, 3.0f);

    SECTION("Iteration over components") {
        SECTION("Single component iteration") {
            int count = 0;
            int sum = 0;
            for(auto &&[entity, comp]: registry.each<dummy>()) {
                count++;
                sum += comp->value;
            }
            REQUIRE(count == 3);
            REQUIRE(sum == 60);
        }

        SECTION("Multiple component iteration") {
            int count = 0;

            for(auto &&[entity, d, s]: registry.each<st::mut<dummy>, st::mut<second_comp>>()) {
                count++;
                REQUIRE(((d->value == 10 && s->value == 1.0f) || (d->value == 30 && s->value == 3.0f)));

                d->value *= 2;
                s->value *= 10.0f;
            }

            REQUIRE(count == 2);

            auto d1 = registry.get<dummy>(en1);
            auto s1 = registry.get<second_comp>(en1);
            REQUIRE(d1->value == 20);
            REQUIRE(s1->value == 10.0f);

            auto d3 = registry.get<dummy>(en3);
            auto s3 = registry.get<second_comp>(en3);
            REQUIRE(d3->value == 60);
            REQUIRE(s3->value == 30.0f);
        }

        SECTION("Entity only iteration") {
            auto view = registry.view<dummy, second_comp>();
            std::vector<st::entity> ens;
            for(auto en: view) {
                ens.emplace_back(en);
            }
            REQUIRE_THAT(ens, UnorderedRangeEquals({en1, en3}));
        }

        SECTION("Exclude iteration") {
            SECTION("Each") {
                auto each = registry.each<st::mut<dummy>>(st::exclude<second_comp, int>);
                auto it = each.begin();
                REQUIRE(std::get<0>(*it) == en2);
                ++it;
                REQUIRE(it == each.end());
            }
            SECTION("View") {
                auto view = registry.view<dummy>(st::exclude<second_comp, int>);
                auto it = view.begin();
                REQUIRE(*it == en2);
                ++it;
                REQUIRE(it == view.end());
            }
        }
    }

    SECTION("Sort") {
        SECTION("Sort") {
            registry.sort<dummy>([](const dummy &lhs, const dummy &rhs) {
                return lhs.value > rhs.value;
            });

            std::vector<int> values;
            for(auto &&[entity, comp]: registry.each<dummy>()) {
                values.push_back(comp->value);
            }

            REQUIRE(std::ranges::is_sorted(values, std::greater<int>{}));
        }
        SECTION("Sort by entity") {
            registry.sort<dummy>([&registry](const st::entity &lhs, const st::entity &rhs) {
                return registry.get<dummy>(lhs)->value > registry.get<dummy>(rhs)->value;
            });

            std::vector<int> values;
            for(auto &&[entity, comp]: registry.each<dummy>()) {
                values.push_back(comp->value);
            }

            REQUIRE(std::ranges::is_sorted(values, std::greater<int>{}));
        }
    }
}

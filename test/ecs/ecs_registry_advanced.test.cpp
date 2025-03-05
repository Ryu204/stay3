#include <catch2/catch_all.hpp>

import stay3.ecs;

namespace {

struct test_event_tracker {
    int construct_count{};
    int update_count{};
    int destroy_count{};
    void on_construct(st::ecs_registry &, st::entity) {
        this->construct_count++;
    }

    void on_update(st::ecs_registry &, st::entity) {
        this->update_count++;
    }

    void on_destroy(st::ecs_registry &, st::entity) {
        this->destroy_count++;
    }
};

struct complex_component {
    std::string name;
    int value;

    complex_component(std::string name, int val)
        : name{std::move(name)}, value{val} {}
};

struct dummy {
    int value;
};

} // namespace

TEST_CASE("Advanced Entity and Component Scenarios") {
    st::ecs_registry registry;

    SECTION("Multiple Entity Creation and Destruction") {
        constexpr auto NUM_ENTITIES = 100;
        std::vector<st::entity> entities;

        for(int i = 0; i < NUM_ENTITIES; ++i) {
            auto en = registry.create_entity();
            entities.push_back(en);
            registry.add_component<dummy>(en, i * 10);
        }

        for(auto en: entities) {
            REQUIRE(registry.contains_entity(en));
        }

        for(size_t i = 0; i < entities.size(); i += 2) {
            registry.destroy_entity(entities[i]);
        }

        for(size_t i = 0; i < entities.size(); ++i) {
            if(i % 2 == 0) {
                REQUIRE_FALSE(registry.contains_entity(entities[i]));
            } else {
                REQUIRE(registry.contains_entity(entities[i]));
            }
        }
    }

    SECTION("Component Event Tracking") {
        auto en = registry.create_entity();
        test_event_tracker tracker;

        SECTION("Construct Event") {
            registry.on<st::comp_event::construct, dummy>()
                .connect<&test_event_tracker::on_construct>(tracker);

            registry.add_component<dummy>(en);

            REQUIRE(tracker.construct_count == 1);
        }

        SECTION("Update Event") {
            registry.add_component<dummy>(en);

            registry.on<st::comp_event::update, dummy>()
                .connect<&test_event_tracker::on_update>(tracker);

            registry.patch_component<dummy>(en, [](dummy &dum) {
                dum.value = 42;
            });
            REQUIRE(tracker.update_count == 1);

            registry.replace_component<dummy>(en, 100);
            REQUIRE(tracker.update_count == 2);

            SECTION("Read and write proxy") {
                {
                    auto dum = registry.get_component<dummy>(en);
                    dum->value = 0;
                    // Out of scope
                }
                REQUIRE(tracker.update_count == 3);
                REQUIRE(registry.get_component<const dummy>(en)->value == 0);

                int do_not_optimize_out{};
                {
                    auto dum = registry.get_component<const dummy>(en);
                    do_not_optimize_out = dum->value;
                    // Out of scope
                }
                REQUIRE(do_not_optimize_out == 0);
                REQUIRE(tracker.update_count == 3);
            }

            SECTION("Each iteration") {
                REQUIRE(tracker.update_count == 2);
                registry.add_component<complex_component>(en, "hello", 42);

                for(auto [e, d, cd]: registry.each<dummy, const complex_component>()) {
                    d->value += cd->value;
                }
                REQUIRE(tracker.update_count == 3);

                registry.on<st::comp_event::update, complex_component>()
                    .connect<&test_event_tracker::on_update>(tracker);
                for(auto [e, d, cd]: registry.each<const dummy, complex_component>()) {
                    cd->value += d->value;
                }
                REQUIRE(tracker.update_count == 4);

                int do_not_optimize_out{};
                for(auto [e, d, cd]: registry.each<const dummy, const complex_component>()) {
                    do_not_optimize_out += cd->value + d->value;
                }
                REQUIRE(tracker.update_count == 4);
                REQUIRE(do_not_optimize_out != 0);
            }
        }

        SECTION("Destroy Event") {
            registry.add_component<dummy>(en);

            registry.on<st::comp_event::destroy, dummy>()
                .connect<&test_event_tracker::on_destroy>(tracker);

            registry.remove_component<dummy>(en);
            REQUIRE(tracker.destroy_count == 1);
        }
    }

    SECTION("Multiple Proxy and Access Scenarios") {
        auto en = registry.create_entity();

        SECTION("Multiple Component Access") {
            registry.add_component<dummy>(en, 10);
            registry.add_component<complex_component>(en, "multi", 20);

            auto [e, d, c] = *registry.each<dummy, complex_component>().begin();

            REQUIRE(d->value == 10);
            REQUIRE(c->name == "multi");
            REQUIRE(c->value == 20);

            d->value = 100;
            c->value = 200;

            auto rd = registry.get_component<const dummy>(en);
            auto rc = registry.get_component<const complex_component>(en);

            REQUIRE(rd->value == 100);
            REQUIRE(rc->value == 200);
        }
    }
}
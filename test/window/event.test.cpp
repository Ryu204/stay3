#include <catch2/catch_test_macros.hpp>

import stay3;

using namespace st;

TEST_CASE("event type check") {
    event e;

    SECTION("Initial type should be none") {
        REQUIRE(e.is<event::none>());
    }
    SECTION("Different type assignment should change type") {
        e = event::close_requested{};
        REQUIRE(e.is<event::close_requested>());
        REQUIRE_FALSE(e.is<event::event_b>());
    }
    SECTION("Is event type should return correct result") {
        STATIC_REQUIRE_FALSE(event::is_event_type<app>);
        STATIC_REQUIRE(event::is_event_type<event::close_requested>);
    }
}

TEST_CASE("event get if") {
    event e;

    SECTION("Should return correct values") {
        constexpr auto foo_val = 10;
        e = event::event_b{.foo = foo_val};
        const auto *maybe_a = e.try_get<event::close_requested>();
        const auto *maybe_b = e.try_get<event::event_b>();
        REQUIRE_FALSE(maybe_a);
        REQUIRE(maybe_b);
        REQUIRE(maybe_b->foo == foo_val);
    }
}

TEST_CASE("constructor should set correct value") {
    event e{event::event_b{.foo = 1}};

    REQUIRE(e.is<event::event_b>());
    REQUIRE(e.try_get<event::event_b>()->foo == 1);
}
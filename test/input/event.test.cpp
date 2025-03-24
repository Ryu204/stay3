#include <catch2/catch_test_macros.hpp>

import stay3;

using namespace st;

TEST_CASE("event type check") {
    event ev;

    SECTION("Initial type should be none") {
        REQUIRE(ev.is<event::none>());
    }
    SECTION("Different type assignment should change type") {
        ev = event::close_requested{};
        REQUIRE(ev.is<event::close_requested>());
        REQUIRE_FALSE(ev.is<event::key_released>());
    }
    SECTION("Is event type should return correct result") {
        STATIC_REQUIRE_FALSE(event::is_event_type<app>);
        STATIC_REQUIRE(event::is_event_type<event::close_requested>);
    }
}

TEST_CASE("event get if") {
    event ev;

    SECTION("Should return correct values") {
        ev = event::key_pressed{.code = scancode::a};
        const auto *maybe_closed = ev.try_get<event::close_requested>();
        const auto *maybe_pressed = ev.try_get<event::key_pressed>();
        REQUIRE_FALSE(maybe_closed);
        REQUIRE(maybe_pressed);
        REQUIRE(maybe_pressed->code == scancode::a);
    }
}

TEST_CASE("constructor should set correct value") {
    event ev{event::key_released{.code = scancode::num_1}};

    REQUIRE(ev.is<event::key_released>());
    REQUIRE(ev.try_get<event::key_released>()->code == scancode::num_1);
}
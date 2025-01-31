#include <chrono>
#include <thread>
#include <catch2/catch_test_macros.hpp>

import stay3;

using namespace st;

constexpr auto sleep_ms = 100;
constexpr auto acceptable_margin_ms = 50;
constexpr auto ms_per_s = 1000.F;

TEST_CASE("stop_watch time_since_start") {
    stop_watch watch;

    SECTION("Initial timestamp should be within [0,1] seconds") {
        const seconds first_timestamp = watch.time_since_start();
        REQUIRE(first_timestamp >= 0.F);
        REQUIRE(first_timestamp <= 1.F);
    }

    SECTION("Time progresses after sleep") {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        const seconds time_after_sleep = watch.time_since_start();
        REQUIRE(time_after_sleep > 0.F);
        REQUIRE(time_after_sleep >= sleep_ms / ms_per_s);
        REQUIRE(time_after_sleep <= (sleep_ms + acceptable_margin_ms) / ms_per_s);
    }
}

TEST_CASE("stop_watch restart behavior") {
    stop_watch watch;

    SECTION("Restart should reset time close to zero") {
        const auto time_before_restart = watch.restart();
        REQUIRE(time_before_restart >= 0.F);
        REQUIRE(time_before_restart <= 1.F);

        const auto time_after_restart = watch.restart();
        REQUIRE(time_after_restart >= 0.F);
        REQUIRE(time_after_restart <= 1.F);
    }

    SECTION("Restart after sleep should return elapsed time") {
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        const auto time_after_sleep = watch.restart();
        REQUIRE(time_after_sleep >= sleep_ms / ms_per_s);
        REQUIRE(time_after_sleep <= (sleep_ms + acceptable_margin_ms) / ms_per_s);
    }
}

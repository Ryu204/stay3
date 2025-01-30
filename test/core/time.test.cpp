#include <cassert>
#include <chrono>
#include <thread>

import stay3;

using namespace st;

constexpr auto sleep_ms = 100;
constexpr auto acceptable_margin_ms = 50;
constexpr auto ms_per_s = 1000.F;

void test_stop_watch_time_since_start() {
    stop_watch watch;

    const seconds first_timestamp = watch.time_since_start();
    assert(first_timestamp >= 0.F && first_timestamp <= 1.F && "Initial time_since_start should not be in [0, 1]");

    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    const seconds time_after_sleep = watch.time_since_start();
    assert(time_after_sleep > first_timestamp && "Time should increase after sleep");

    assert(
        time_after_sleep >= sleep_ms / ms_per_s
        && time_after_sleep <= (sleep_ms + acceptable_margin_ms) / ms_per_s
        && "Time should be within expected range after sleep");
}

void test_stop_watch_restart() {
    stop_watch watch;

    const auto time_before_restart = watch.restart();
    const auto time_after_restart = watch.restart();

    assert(time_after_restart >= 0.F && time_after_restart <= 1.F && "Time after restart should be in [0, 1]");

    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    const auto time_after_second_sleep = watch.restart();
    assert(
        time_after_second_sleep >= sleep_ms / ms_per_s
        && time_after_second_sleep <= (sleep_ms + acceptable_margin_ms) / ms_per_s
        && "Time after sleep should be a bit more than sleep time");
}

int main() {
    test_stop_watch_time_since_start();
    test_stop_watch_restart();

    return 0;
}

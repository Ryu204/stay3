module;

#include <iostream>

module stay3.core;

import :time;

constexpr auto temp_fps = 60.F;

namespace st {

void app::run() {
    stop_watch app_stop_watch;
    auto elapsed_time = app_stop_watch.restart();
    constexpr seconds time_per_update = 1.F / temp_fps;
    while(true) {
        elapsed_time += app_stop_watch.restart();
        while(elapsed_time > time_per_update) {
            elapsed_time -= time_per_update;
            input();
            update(time_per_update);
        }
        render();
    }
}

void app::update(seconds dt) {
    std::cout << dt << '\n';
}

void app::render() {
}

void app::input() {
}

} // namespace st

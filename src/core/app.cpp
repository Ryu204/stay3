module;

#include <iostream>
#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif
module stay3.core;

import :time;

constexpr auto temp_fps = 60.F;

namespace st {

void app::run() {
#ifdef __EMSCRIPTEN__
    constexpr auto func = [](void *p_app) {
        static_cast<app *>(p_app)->on_frame();
    };
    emscripten_set_main_loop_arg(func, static_cast<void *>(this), 0, true);
#else
    while(true) {
        on_frame();
    }
#endif
}

void app::on_frame() {
    const auto elapsed_time = m_watch.restart();
    m_pending_time += elapsed_time;
    constexpr seconds time_per_update = 1.F / temp_fps;
    while(m_pending_time > time_per_update) {
        m_pending_time -= time_per_update;
        input();
        update(time_per_update);
    }
    render();
}

void app::update(seconds dt) {
    std::cout << dt << '\n';
}

void app::render() {
}

void app::input() {
}

} // namespace st

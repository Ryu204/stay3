module;

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif
module stay3.core;

import stay3.window;

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
    while(m_window.is_open()) {
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
        if(input() == window_closed::yes) {
            return;
        };
        update(time_per_update);
    }
    render();
}

void app::update(seconds delta) {
}

void app::render() {
    m_window.clear();
    m_window.display();
}

app::window_closed app::input() {
    while(true) {
        const auto ev = m_window.poll_event();
        if(ev.is<event::none>()) {
            break;
        }
        if(ev.is<event::close_requested>()) {
            m_window.close();
            return app::window_closed::yes;
        }
    }
    return app::window_closed::no;
}

} // namespace st

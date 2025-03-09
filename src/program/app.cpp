module;

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

module stay3.program;

import stay3.ecs;
import stay3.node;
import stay3.window;
import stay3.core;
import stay3.systems;

constexpr auto temp_fps = 60.F;

namespace st {

system_manager<tree_context> &app::systems() {
    return m_ecs_systems;
}

app &app::enable_default_systems() {
    m_ecs_systems
        .add<transform_sync_system>()
        .run_as<sys_type::start>(sys_priority::very_high)
        .run_as<sys_type::post_update>(sys_priority::very_high);

    return *this;
}

void app::run() {
    m_ecs_systems.start(m_tree_context);
    while(m_window.is_open()) {
        on_frame();
#ifdef __EMSCRIPTEN__
        emscripten_sleep(100);
#endif
    }
    m_ecs_systems.cleanup(m_tree_context);
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
        if(update(time_per_update) == should_exit::yes) {
            m_window.close();
            return;
        }
        render();
    }
}

app::should_exit app::update(seconds delta) {
    auto update_res = m_ecs_systems.update(delta, m_tree_context);
    auto post_update_res = m_ecs_systems.post_update(delta, m_tree_context);

    if(update_res == sys_run_result::exit || post_update_res == sys_run_result::exit) {
        return should_exit::yes;
    }
    return should_exit::no;
}

void app::render() {
    m_window.clear();
    m_ecs_systems.render(m_tree_context);
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

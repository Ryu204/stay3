module;

#include <filesystem>

#ifdef __EMSCRIPTEN__
#    include <emscripten.h>
#endif

module stay3.program;

import stay3.ecs;
import stay3.node;
import stay3.input;
import stay3.core;
import stay3.systems;
import :config;

namespace st {

app::app(const app_config &config)
    : m_window{config.window}, m_time_per_update{1.F / config.updates_per_second}, m_config{config} {
    if(config.use_default_systems) {
        enable_default_systems();
    }
}

app::~app() {
    // Zero the tree, then systems can be safely destroyed
    // If this step is skipped, the destroyed systems cannot handle `ecs_registry`'s signals
    m_tree_context.destroy_tree();
}

system_manager<tree_context> &app::systems() {
    return m_ecs_systems;
}

app &app::enable_default_systems() {
    m_ecs_systems
        .add<transform_sync_system>()
        .run_as<sys_type::start>(sys_priority::very_high)
        .run_as<sys_type::post_update>(sys_priority::very_low);
    m_ecs_systems
        .add<render_system>(m_window.size(), std::filesystem::path{m_config.assets_dir} / "shaders" / "my_shader.wgsl", m_config.render)
        .run_as<sys_type::start>(sys_priority::very_high)
        .run_as<sys_type::render>()
        .run_as<sys_type::cleanup>(sys_priority::very_low);
    m_ecs_systems
        .add<text_system>()
        .run_as<sys_type::start>(sys_priority::high)
        .run_as<sys_type::render>(sys_priority::high);
    auto physics =
        m_ecs_systems
            .add<physics_system>(m_config.physics)
            .run_as<sys_type::start>(sys_priority::very_high)
            .run_as<sys_type::update>(sys_priority::very_high);
    if(m_config.physics.debug_draw) {
        physics.run_as<sys_type::render>(sys_priority::very_high);
    }
    m_ecs_systems
        .add<lua_script_system>()
        .run_as<sys_type::start>(sys_priority::very_low)
        .run_as<sys_type::update>(sys_priority::very_low)
        .run_as<sys_type::post_update>(sys_priority::very_low)
        .run_as<sys_type::input>(sys_priority::very_low)
        .run_as<sys_type::cleanup>(sys_priority::very_high);

    return *this;
}

void app::run() {
    add_runtime_info();
    m_ecs_systems.start(m_tree_context);
#ifndef __EMSCRIPTEN__
    while(m_window.is_open()) {
        on_frame();
    }
#else
    if(m_config.web.exit_main) {
        emscripten_set_main_loop_arg(
            /* callback */ +[](void *this_app) {
            auto casted_app = static_cast<app *>(this_app);
            if(casted_app->m_window.is_open()) { 
                casted_app->on_frame(); 
            } else {
                emscripten_cancel_main_loop();
                delete casted_app;
            } },
            /* arg */ static_cast<void *>(this),
            /* fps */ 0,
            /* simulate_infinite_loop */ false);
    } else {
        while(m_window.is_open()) {
            constexpr auto sleep_milli = 100;
            on_frame();
            emscripten_sleep(sleep_milli);
        }
    }
#endif
}

void app::on_frame() {
    const auto elapsed_time = m_watch.restart();
    m_pending_time += elapsed_time;
    while(m_pending_time > m_time_per_update) {
        m_pending_time -= m_time_per_update;
        if(input() == window_closed::yes) {
            return;
        };
        if(update(m_time_per_update) == should_exit::yes) {
            close_window();
            return;
        }
        render();
    }
}

void app::add_runtime_info() {
    m_tree_context.vars().emplace<runtime_info>(m_window);
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
    m_ecs_systems.render(m_tree_context);
}

app::window_closed app::input() {
    while(true) {
        const auto ev = m_window.poll_event();
        if(ev.is<event::none>()) {
            break;
        }
        if(ev.is<event::close_requested>()) {
            close_window();
            return app::window_closed::yes;
        }
        if(m_ecs_systems.input(ev, m_tree_context) == sys_run_result::exit) {
            close_window();
            return app::window_closed::yes;
        }
    }
    return app::window_closed::no;
}

void app::close_window() {
    m_ecs_systems.cleanup(m_tree_context);
    m_window.close();
}

} // namespace st

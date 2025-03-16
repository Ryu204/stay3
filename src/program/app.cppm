module;

#include <cstdint>

export module stay3.program:app;

import stay3.ecs;
import stay3.node;
import stay3.graphics;
import stay3.core;
import stay3.system.render;
import :config;

export namespace st {
class app {
public:
    app(const app_config &config = {});
    app &enable_default_systems();
    system_manager<tree_context> &systems();
    void run();

private:
    enum class window_closed : std::uint8_t {
        yes,
        no,
    };
    enum class should_exit : std::uint8_t {
        yes,
        no
    };

    void add_runtime_info();
    should_exit update(seconds delta);
    void on_frame();
    window_closed input();
    void render();
    void close_window();

    stop_watch m_watch;
    seconds m_pending_time{0.F};
    seconds m_time_per_update;
    unsigned int m_emscripten_sleep_milli;

    glfw_window m_window;

    tree_context m_tree_context;
    system_manager<tree_context> m_ecs_systems;
    render_config m_render_config;
};
} // namespace st

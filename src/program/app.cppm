module;

#include <cstdint>

export module stay3.program:app;

import stay3.ecs;
import stay3.node;
import stay3.graphics.core;
import stay3.core;
import stay3.physics;
import stay3.system.render;
import :config;

export namespace st {
class app {
public:
    app(const app_config &config = {});
    ~app();
    app(const app &) = delete;
    app &operator=(const app &) = delete;
    app(app &&) noexcept = delete;
    app &operator=(app &&) noexcept = delete;
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

    app &enable_default_systems();
    void add_runtime_info();
    should_exit update(seconds delta);
    void on_frame();
    window_closed input();
    void render();
    void close_window();

    stop_watch m_watch;
    seconds m_pending_time{0.F};
    seconds m_time_per_update;

    glfw_window m_window;

    system_manager<tree_context> m_ecs_systems;
    tree_context m_tree_context;
    app_config m_config;
};
} // namespace st

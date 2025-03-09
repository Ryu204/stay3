module;

#include <cstdint>

export module stay3.program:app;

import stay3.ecs;
import stay3.node;
import stay3.graphics;
import stay3.core;

export namespace st {
class app {
public:
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

    should_exit update(seconds delta);
    void on_frame();
    window_closed input();
    void render();

    stop_watch m_watch;
    seconds m_pending_time{0.F};

    glfw_window m_window;

    tree_context m_tree_context;
    system_manager<tree_context> m_ecs_systems;
};
} // namespace st

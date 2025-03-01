module;

#include <cstdint>

export module stay3.core:app;

import stay3.ecs;
import stay3.node;
import stay3.graphics;
import stay3.system;

export namespace st {
class app {
public:
    system_manager<tree_context> &systems();
    void run();

private:
    enum class window_closed : std::uint8_t {
        yes,
        no,
    };

    void update(seconds delta);
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

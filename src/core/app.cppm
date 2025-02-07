export module stay3.core:app;

import stay3.graphics;

import :time;

export namespace st {
class app {
public:
    void run();

private:
    enum class window_closed {
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
};
} // namespace st

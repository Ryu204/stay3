export module stay3.core:app;

import :time;

export namespace st {
class app {
public:
    void run();

private:
    void update(seconds delta);
    void on_frame();
    void input();
    void render();

    stop_watch m_watch;
    seconds m_pending_time{0.F};
};
} // namespace st

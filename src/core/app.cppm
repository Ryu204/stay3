export module stay3.core:app;

import :time;

export namespace st {
class app {
public:
    void run();

private:
    void update(seconds delta);
    void input();
    void render();
};
} // namespace st

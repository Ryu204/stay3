module;

#include <memory>
#include <queue>
#include <GLFW/glfw3.h>

export module stay3.graphics:glfw_window;

import stay3.window;

namespace st {

struct glfw_context;
class glfw_context_user {
public:
    glfw_context_user();

private:
    std::shared_ptr<glfw_context> m_context;
};

export class glfw_window: public base_window, private st::glfw_context_user {
public:
    glfw_window();
    ~glfw_window() override;

    glfw_window(const glfw_window &) = delete;
    glfw_window &operator=(const glfw_window &) = delete;
    glfw_window(glfw_window &&other) noexcept;
    glfw_window &operator=(glfw_window &&) noexcept;

    [[nodiscard]] bool is_open() const override;
    void close() override;
    void clear() override;
    void display() override;
    event poll_event() override;

private:
    void own_glfw_user_pointer();
    void set_window_callbacks();

    GLFWwindow *m_window{};
    std::queue<event> m_event_queue;
};

} // namespace st
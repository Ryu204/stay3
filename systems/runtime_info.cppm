module;

#include <functional>

export module stay3.system.runtime_info;

import stay3.graphics.core;

export namespace st {
class runtime_info {
public:
    runtime_info(glfw_window &window)
        : m_window{window} {}
    [[nodiscard]] glfw_window &window() {
        return m_window;
    }

private:
    std::reference_wrapper<glfw_window> m_window;
};
} // namespace st
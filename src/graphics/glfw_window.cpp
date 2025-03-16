module;

#include <cassert>
#include <memory>
#include <mutex>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu_glfw.h>

module stay3.graphics;

import stay3.window;
import stay3.core;
import :error;

namespace st {
struct glfw_context {
    glfw_context() {
        if(glfwInit() == 0) {
            throw graphics_error{"Failed to init glfw."};
        }
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    }

    ~glfw_context() {
        glfwTerminate();
    }

    glfw_context(const glfw_context &) = delete;
    glfw_context(glfw_context &&) = delete;
    glfw_context &operator=(const glfw_context &) = delete;
    glfw_context &operator=(glfw_context &&) = delete;
};

glfw_context_user::glfw_context_user()
    : m_context{[]() {
          static std::mutex mutex;
          static std::weak_ptr<glfw_context> weak_context;

          const std::lock_guard lock{mutex};
          auto context = weak_context.lock();
          if(!context) {
              context = std::make_shared<glfw_context>();
              weak_context = context;
          }
          return context;
      }()} {}

glfw_window::glfw_window(const window_config &config)
    : m_window{
          glfwCreateWindow(static_cast<int>(config.size.x), static_cast<int>(config.size.y), config.name.c_str(), nullptr, nullptr)} {
    if(m_window == nullptr) {
        throw graphics_error{"Failed to create glfw window."};
    }
    own_glfw_user_pointer();
    set_window_callbacks();
}

glfw_window::glfw_window(glfw_window &&other) noexcept {
    auto *other_handle = other.m_window;
    other.m_window = nullptr;
    m_window = other_handle;

    if(m_window != nullptr) {
        own_glfw_user_pointer();
    }
}

glfw_window &glfw_window::operator=(glfw_window &&other) noexcept {
    auto *other_handle = other.m_window;
    other.m_window = nullptr;
    if(m_window != nullptr) {
        glfwDestroyWindow(m_window);
    }
    m_window = other_handle;

    if(m_window != nullptr) {
        own_glfw_user_pointer();
    }
    return *this;
}

glfw_window::~glfw_window() {
    if(m_window != nullptr) {
        glfwDestroyWindow(m_window);
    }
}

bool glfw_window::is_open() const {
    return m_window != nullptr;
}

void glfw_window::close() {
    assert(m_window && "Null window handle");
    glfwDestroyWindow(m_window);
    m_window = nullptr;
}

void glfw_window::clear() {
    assert(m_window && "Null window handle");
}

void glfw_window::display() {
    assert(m_window && "Null window handle");
}

event glfw_window::poll_event() {
    assert(m_window && "Null window handle");
    glfwPollEvents();
    if(m_event_queue.empty()) {
        return event::none{};
    }
    auto result = m_event_queue.front();
    m_event_queue.pop();
    return result;
}

void glfw_window::set_window_callbacks() {
    assert(m_window && "Null window handle");

    constexpr auto on_close_requested = [](GLFWwindow *window) -> void {
        auto *this_ptr = static_cast<glfw_window *>(glfwGetWindowUserPointer(window));
        this_ptr->m_event_queue.emplace(event::close_requested{});
    };
    glfwSetWindowCloseCallback(m_window, on_close_requested);
}

void glfw_window::own_glfw_user_pointer() {
    assert(m_window && "Null window handle");

    glfwSetWindowUserPointer(m_window, static_cast<void *>(this));
}

wgpu::Surface glfw_window::create_wgpu_surface(const wgpu::Instance &instance) {
    return wgpu::glfw::CreateSurfaceForWindow(instance, m_window);
}

vec2u glfw_window::size() const {
    assert(m_window && "Null window handle");
    int width{};
    int height{};
    glfwGetWindowSize(m_window, &width, &height);
    return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
}

} // namespace st
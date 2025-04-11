module;

#include <cassert>
#include <memory>
#include <mutex>
#include <GLFW/glfw3.h>
#ifdef __EMSCRIPTEN__

#else
#    include <webgpu/webgpu_glfw.h>
#endif
module stay3.graphics;

import stay3.input;
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

key_status glfw_window::get_key(scancode code) {
    switch(glfwGetKey(m_window, static_cast<int>(code))) {
    case GLFW_PRESS:
        return key_status::pressed;
    case GLFW_RELEASE:
        return key_status::released;
    default:
        break;
    }
    assert(false && "Unhandled key status");
    return key_status::released;
}

void glfw_window::set_window_callbacks() {
    assert(m_window && "Null window handle");

    constexpr auto on_close_requested = [](GLFWwindow *window) -> void {
        auto *this_ptr = static_cast<glfw_window *>(glfwGetWindowUserPointer(window));
        this_ptr->m_event_queue.emplace(event::close_requested{});
    };
    glfwSetWindowCloseCallback(m_window, on_close_requested);
    constexpr auto on_key_pressed =
        // NOLINTNEXTLINE(*-easily-swappable-parameters)
        [](GLFWwindow *window, int key, [[maybe_unused]] int scan, int action, [[maybe_unused]] int mods) {
            auto *this_ptr = static_cast<glfw_window *>(glfwGetWindowUserPointer(window));
            switch(action) {
            case GLFW_PRESS:
                this_ptr->m_event_queue.emplace(event::key_pressed{static_cast<scancode>(key)});
                break;
            case GLFW_RELEASE:
                this_ptr->m_event_queue.emplace(event::key_released{static_cast<scancode>(key)});
                break;
            default:
                break;
            }
        };
    glfwSetKeyCallback(m_window, on_key_pressed);
}

void glfw_window::own_glfw_user_pointer() {
    assert(m_window && "Null window handle");

    glfwSetWindowUserPointer(m_window, static_cast<void *>(this));
}

wgpu::Surface glfw_window::create_wgpu_surface(const wgpu::Instance &instance) {
#ifdef __EMSCRIPTEN__

#else
    return wgpu::glfw::CreateSurfaceForWindow(instance, m_window);
#endif
}

vec2u glfw_window::size() const {
    assert(m_window && "Null window handle");
    int width{};
    int height{};
    glfwGetWindowSize(m_window, &width, &height);
    return {static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
}

} // namespace st
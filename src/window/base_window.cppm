export module stay3.window:base_window;

import :event;

export namespace st {
class base_window {
public:
    base_window() = default;
    virtual ~base_window() = default;
    base_window(const base_window &) = delete;
    base_window(base_window &&) = delete;
    base_window &operator=(const base_window &) = delete;
    base_window &operator=(base_window &&) = delete;

    [[nodiscard]] virtual bool is_open() const = 0;
    virtual void close() = 0;
    virtual void clear() = 0;
    virtual void display() = 0;
    virtual event poll_event() = 0;
};
} // namespace st
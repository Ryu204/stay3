module;

#include <chrono>
#include <utility>

module stay3.system;

namespace st {
stop_watch::stop_watch()
    : m_start_time{std_clock::now()}, m_last_restart_time{m_start_time} {}

seconds stop_watch::restart() {
    auto now = std_clock::now();
    const auto secs = std::chrono::duration<float>(now - m_last_restart_time).count();
    m_last_restart_time = std::move(now);
    return secs;
}

seconds stop_watch::time_since_start() const {
    return std::chrono::duration<float>(std_clock::now() - m_start_time).count();
}
} // namespace st

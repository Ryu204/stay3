module;

#include <chrono>

export module stay3.core:time;

export namespace st {

using seconds = float;
class stop_watch {
public:
    stop_watch();
    /**
     * @brief Returns time since last restart
     */
    seconds restart();
    /**
     * @brief Returns time since constructor
     */
    [[nodiscard]] seconds time_since_start() const;

private:
    using std_clock = std::chrono::steady_clock;
    using time_point = std_clock::time_point;

    time_point m_start_time;
    time_point m_last_restart_time;
};
} // namespace st

module;

#include <cstdint>
#include <iostream>
#include <webgpu/webgpu_cpp.h>

export module stay3.core:logger;

export namespace st::log {

std::ostream &operator<<(std::ostream &os, const wgpu::StringView &mes) {
    os << std::string_view{mes.data, mes.length};
    return os;
}

enum class level : std::uint8_t {
    info,
    warn,
    error,
};

template<typename... ts>
void write(level lvl, ts &&...messages) {
    auto &stream = (lvl == level::error) ? std::cerr : std::cout;

    switch(lvl) {
    case level::info:
        stream << "[INFO] ";
        break;
    case level::warn:
        stream << "[WARN] ";
        break;
    case level::error:
        stream << "[ERROR] ";
        break;
    }

    (stream << ... << std::forward<ts>(messages)) << std::endl;
}

template<typename... ts>
void info(ts &&...messages) {
    write(level::info, std::forward<ts>(messages)...);
}

template<typename... ts>
void warn(ts &&...messages) {
    write(level::warn, std::forward<ts>(messages)...);
}

template<typename... ts>
void error(ts &&...messages) {
    write(level::error, std::forward<ts>(messages)...);
}

} // namespace st::log
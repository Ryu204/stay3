module;

#include <cstdint>
#include <filesystem>
#include <format>
#include <string>
#include <utility>
#include <variant>

export module stay3.system.script:script_component;

import stay3.core;

struct script_location_file {
    std::filesystem::path file;
};
struct script_location_memory {
    const char *identifier;
};
using script_location = std::variant<script_location_file, script_location_memory>;

export namespace st {
class script_component {
public:
    using id_type = std::uint32_t;
    using path = std::filesystem::path;
    using script_name = std::string;

    script_component(id_type id, path filepath, script_name name)
        : m_id{id},
          m_name{std::move(name)},
          m_location{
              script_location_file{std::move(filepath)}} {}

    script_component(id_type id, const char *identifier, script_name name)
        : m_id{id},
          m_name{std::move(name)},
          m_location{script_location_memory{identifier}} {}

    [[nodiscard]] id_type id() const {
        return m_id;
    }
    [[nodiscard]] [[nodiscard]] std::string location() const {
        return std::visit(
            visit_helper{
                [](const script_location_file &file) { return std::format("file:{}", file.file.string()); },
                [](const script_location_memory &mem) { return std::format("memory:{}", mem.identifier); }},
            m_location);
    }

    [[nodiscard]] const script_name &name() const {
        return m_name;
    }

private:
    id_type m_id;
    script_name m_name;
    script_location m_location;
};

bool operator==(const script_component &lhs, const script_component &rhs) {
    return lhs.id() == rhs.id();
}
} // namespace st
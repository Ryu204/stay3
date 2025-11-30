module;

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

export module stay3.system.script:script_component;

export namespace st {
class script_component {
public:
    using id_type = std::uint32_t;
    using path = std::filesystem::path;
    using script_name = std::string;

    script_component(id_type id, path filepath, script_name name): m_id{id}, m_path{std::move(filepath)}, m_name{std::move(name)} {}
    [[nodiscard]] id_type id() const {
        return m_id;
    }
    [[nodiscard]] const path &filepath() const {
        return m_path;
    }
    [[nodiscard]] const script_name &name() const {
        return m_name;
    }

private:
    id_type m_id;
    path m_path;
    script_name m_name;
};

bool operator==(const script_component &lhs, const script_component &rhs) {
    return lhs.id() == rhs.id();
}
} // namespace st
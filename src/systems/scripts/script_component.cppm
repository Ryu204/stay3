module;

#include <cstdint>
#include <filesystem>
#include <utility>

export module stay3.system.script:script_component;

export namespace st {
class script_component {
public:
    using id_type = std::uint32_t;
    using path = std::filesystem::path;
    script_component(id_type id, path filepath): m_id{id}, m_path{std::move(filepath)} {}
    [[nodiscard]] id_type id() const {
        return m_id;
    }
    [[nodiscard]] const path &filepath() const {
        return m_path;
    }

private:
    id_type m_id;
    path m_path;
};

bool operator==(const script_component &lhs, const script_component &rhs) {
    return lhs.id() == rhs.id();
}
} // namespace st
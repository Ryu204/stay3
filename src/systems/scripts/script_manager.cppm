module;

#include <cassert>
#include <type_traits>
#include <unordered_set>
#include <variant>
#include <vector>

export module stay3.system.script:script_manager;
import :script_component;
import :script_langs;

namespace st {

export template<script_lang>
class script_system;

/** @brief Record insertion/removal of scripts in a single entity */
export template<script_lang lang>
class script_manager {
    using script_id = script_component::id_type;

    struct add {
        script_id id;
    };
    struct remove {
        script_id id;
    };
    using change = std::variant<add, remove>;
    using change_container = std::vector<change>;
    friend class script_system<lang>;
    [[nodiscard]] bool has_unsaved_changes() const {
        return !m_unsaved_changes.empty();
    }
    void save() {
        m_unsaved_changes.clear();
    }

    [[nodiscard]] const change_container &unsaved_changes() const {
        return m_unsaved_changes;
    }

    void add_single_script(script_id id) {
        assert(!m_current_scripts.contains(id) && "Same script added twice");
        m_current_scripts.insert(id);
        m_unsaved_changes.emplace_back(add{.id = id});
    }
    void remove_single_script(script_id id) {
        assert(m_current_scripts.contains(id) && "Script manager currently does not have this script");
        m_current_scripts.erase(id);
        m_unsaved_changes.emplace_back(remove{.id = id});
    }
    std::unordered_set<script_id> m_current_scripts;
    change_container m_unsaved_changes;

public:
    template<typename... args>
        requires std::conjunction_v<std::is_same<args, script_id>...>
    void add_scripts(args... ids) {
        (add_single_script(ids), ...);
    };
    template<typename... args>
        requires std::conjunction_v<std::is_same<args, script_id>...>
    void remove_scripts(args... ids) {
        (remove_single_script(ids), ...);
    }
};
} // namespace st
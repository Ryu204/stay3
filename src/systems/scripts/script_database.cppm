module;

#include <cassert>
#include <filesystem>
#include <format>
#include <string>
#include <unordered_map>

export module stay3.system.script:script_database;
import stay3.core;
import :script_component;
import :error;

export namespace st {
class script_database {
public:
    script_component::id_type create_script_id() {
        return id_gen.create();
    }
    const script_component &register_script(script_component::id_type created_id, const script_component::path &path, const script_component::script_name &name) {
        assert(id_gen.is_id_active(created_id) && "Id is not valid");
        assert(!components.contains(created_id) && "Id was registered for other script");
        for(auto &&[id, comp]: components) {
            if(std::filesystem::equivalent(path, comp.filepath())) {
                throw st::script_error{"Same script file is registered twice"};
            }
            if(name_to_id.contains(name)) {
                const auto &existing_path = components.at(name_to_id[name]).filepath();
                throw st::script_error{
                    std::format("Same script name is used accross files:\n{}\n{}\n{}", name, path.string(), existing_path.string())};
            }
        }
        auto &&[iter, isOk] = components.try_emplace(created_id, created_id, path, name);
        assert(isOk && "Failed to emplace new script");
        return iter->second;
    }
    void delete_unused_script_id(script_component::id_type id) {
        assert(id_gen.is_id_active(id) && "Id is not created/was deleted");
        id_gen.recycle(id);
    }
    [[nodiscard]] script_component::id_type id_from_name(const script_component::script_name &name) const {
        assert(name_to_id.contains(name) && "Name was not registered");
        return name_to_id.at(name);
    }

private:
    id_generator<script_component::id_type> id_gen;
    std::unordered_map<script_component::id_type, script_component> components;
    std::unordered_map<script_component::script_name, script_component::id_type> name_to_id;
};
} // namespace st
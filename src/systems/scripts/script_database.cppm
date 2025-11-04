module;

#include <cassert>
#include <filesystem>
#include <unordered_map>

export module stay3.system.script:script_database;
import stay3.core;
import :script_component;
import :error;

export namespace st {
class script_database {
public:
    const script_component &register_script(const script_component::path &path) {
        for(auto &&[id, comp]: components) {
            if(std::filesystem::equivalent(path, comp.filepath())) {
                throw st::script_error{"Same script file is registered twice"};
            }
        }
        const auto new_id = id_gen.create();
        auto &&[iter, isOk] = components.try_emplace(new_id, new_id, path);
        assert(isOk && "Failed to emplace new script");
        return iter->second;
    }

private:
    id_generator<script_component::id_type> id_gen;
    std::unordered_map<script_component::id_type, script_component> components;
};
} // namespace st
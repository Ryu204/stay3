module;

#include <optional>
#include <string>

export module stay3.system.script.angelscript:script_info;

import :objects;
import :lifecycle_method;

export namespace st {
struct component_script_info {
    std::string name;
    ags::function factory;
    ags::function on_attached;
    ags::function on_detached;
    std::optional<ags::function> maybe_update{std::nullopt};
    std::optional<ags::function> maybe_post_update{std::nullopt};
    std::optional<ags::function> maybe_input{std::nullopt};

    ags::lifecycle_method_bitmask get_bitmask() {
        ags::lifecycle_method_bitmask result{};
        if(on_attached.get() != nullptr) {
            result |= static_cast<ags::lifecycle_method_bitmask>(ags::lifecycle_method::on_attached);
        }
        if(on_detached.get() != nullptr) {
            result |= static_cast<ags::lifecycle_method_bitmask>(ags::lifecycle_method::on_detached);
        }
        if(maybe_update && maybe_update.value().get() != nullptr) {
            result |= static_cast<ags::lifecycle_method_bitmask>(ags::lifecycle_method::update);
        }
        if(maybe_post_update && maybe_post_update.value().get() != nullptr) {
            result |= static_cast<ags::lifecycle_method_bitmask>(ags::lifecycle_method::post_update);
        }
        if(maybe_input && maybe_input.value().get() != nullptr) {
            result |= static_cast<ags::lifecycle_method_bitmask>(ags::lifecycle_method::input);
        }
        return result;
    }
};
} // namespace st
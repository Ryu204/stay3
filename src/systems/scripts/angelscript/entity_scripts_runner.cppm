module;

#include <bit>
#include <cassert>
#include <cstdint>
#include <format>
#include <optional>
#include <ranges>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>
#include <angelscript.h>

export module stay3.system.script.angelscript:entity_scripts_runner;

import stay3.system.script;
import stay3.ecs;
import stay3.core;
import stay3.node;

import :objects;
import :ops_check;
import :lifecycle_method;
import :script_info;

namespace st::ags {

template<lifecycle_method>
struct method_list {};

template<typename T>
concept script_info_list = std::is_same_v<
    std::remove_reference_t<decltype(std::declval<T>().at(std::declval<script_id>()))>,
    component_script_info>;

// NOLINTNEXTLINE(*-macro-usage)
#define METHOD_LIST_GET_MEMBER(method_name) \
    template<> \
    struct method_list<lifecycle_method::method_name> { \
        template<script_info_list T> \
        static std::optional<function> &get(T &list, script_id id) { \
            return list.at(id).maybe_##method_name; \
        } \
    };

METHOD_LIST_GET_MEMBER(update)
METHOD_LIST_GET_MEMBER(post_update)
METHOD_LIST_GET_MEMBER(input)

#undef METHOD_HOLDER_CONCEPT

export class entity_scripts_runner {
private:
    std::unordered_map<script_id, object> instances;
    std::unordered_map<lifecycle_method, std::unordered_map<script_id, object *>> instances_by_method;

    struct add {
        script_id id;
        object instance;
        lifecycle_method_bitmask lifecycle_methods{};
    };

    struct remove {
        script_id id;
    };
    using change = std::variant<add, remove>;
    std::vector<change> changes;

    enum class comp_loc : std::uint8_t {
        tracked,
        queued,
        not_added,
    };
    [[nodiscard]] std::pair<comp_loc, object *> get_comp(script_id id) {
        for(auto &change: std::ranges::reverse_view(changes)) {
            if(auto *aadd = std::get_if<add>(&change); aadd != nullptr) {
                if(aadd->id == id) {
                    return {comp_loc::queued, &aadd->instance};
                }
            } else {
                auto *rremove = std::get_if<remove>(&change);
                assert(rremove != nullptr && "Must be add or remove");
                return {comp_loc::not_added, nullptr};
            }
        }
        auto it = instances.find(id);
        if(it == instances.end()) {
            return {comp_loc::not_added, nullptr};
        }
        return {comp_loc::tracked, &it->second};
    }

    void add_instance_to_map(script_id id, object &&instance, lifecycle_method_bitmask bitmask) {
        auto &&[it, ok] = instances.emplace(id, std::move(instance));
        assert(ok && "Failed to emplace to map");
        auto *raw_ptr = &(it->second);
        for(auto bit = std::bit_width(bitmask); bit > 0; --bit) {
            auto method = static_cast<lifecycle_method>(1 << bit);
            if((static_cast<lifecycle_method_bitmask>(method) & bitmask) == 0) {
                continue;
            }
            instances_by_method[method].emplace(id, raw_ptr);
        }
    }

    void delete_instance_from_map(script_id id) {
        for(auto &&[method, instance_list]: instances_by_method) {
            instance_list.erase(id);
        }
        instances.erase(id);
    }

public:
    scripts_operation_result attach_component_type(entity en, script_id id, component_script_info &info, asIScriptContext &context) {
        assert(get_comp(id).first == comp_loc::not_added && "Component was already added");
        if(const auto check_result = exec(context, info.factory); !check_result.is_ok) {
            return {
                .error_message = std::format("factory failed: {}", check_result.error_message.value_or("No details")),
                .is_ok = false,
            };
        }
        auto *raw_instance = *(static_cast<asIScriptObject **>(context.GetAddressOfReturnValue()));
        if(raw_instance == nullptr) {
            return {
                .error_message = "Factory did not return an instance, maybe it is not a factory?",
                .is_ok = false,
            };
        }
        object instance{raw_instance};
        if(const auto check_result = exec(context, info.on_attached, instance, en); !check_result.is_ok) {
            return {
                .error_message = std::format("on_attached failed: {}", check_result.error_message.value_or("No details")),
                .is_ok = false,
            };
        }
        this->changes.emplace_back(add{.id = id, .instance = std::move(instance), .lifecycle_methods = info.get_bitmask()});
        return {.is_ok = true};
    }

    scripts_operation_result detach_component_type(script_id id) {
        auto [loc, instance] = get_comp(id);
        assert(loc != comp_loc::not_added && "Component was not added");
        assert(instance != nullptr);
        changes.emplace_back(remove{.id = id});
        return {.is_ok = true};
    }

    [[nodiscard]] bool is_empty() const {
        return instances.empty() && changes.empty();
    }

    template<script_info_list T>
    scripts_operation_result commit_changes(tree_context &tree_ctx, asIScriptContext &script_ctx, T &infos) {
        std::vector<change> snapshot;
        snapshot.swap(changes);
        scripts_operation_result res{.is_ok = true};
        const auto visitor = visit_helper{
            [&is = this->instances, this, &script_ctx, &infos, &tree_ctx, &res](add &ch) {
                assert(!is.contains(ch.id) && "Component was added");
                auto &info = infos.at(ch.id);
                if(info.maybe_start.has_value()) {
                    if(const auto check_result = exec(script_ctx, info.pre_lifecycle_setup, ch.instance, tree_ctx);
                       !check_result.is_ok) {
                        res.merge({
                            .error_message = "Failed to setup pre lifecycle",
                            .is_ok = false,
                        });
                    }
                    if(const auto check_result = exec(script_ctx, info.maybe_start.value(), ch.instance);
                       !check_result.is_ok) {
                        res.merge({
                            .error_message = std::format("Error on script \"start\" method: {}", check_result.error_message.value_or("No details")),
                            .is_ok = false,
                        });
                    }
                }
                this->add_instance_to_map(ch.id, std::move(ch.instance), ch.lifecycle_methods);
            },
            [&is = this->instances, &script_ctx, &infos, this, &res](const remove &ch) {
                const auto it = is.find(ch.id);
                assert(it != is.end() && "Component must be added before removed");
                if(const auto check_result = exec(script_ctx, infos.at(ch.id).on_detached, it->second); !check_result.is_ok) {
                    res.merge({
                        .error_message = std::format("Error on script \"onDetached\" method: {}", check_result.error_message.value_or("No details")),
                        .is_ok = false,
                    });
                }
                this->delete_instance_from_map(ch.id);
            }};
        for(auto &ch: snapshot) {
            std::visit(visitor, ch);
        }
        return res;
    }

    template<lifecycle_method type, script_info_list method_holder, typename... args>
    scripts_operation_result run(tree_context &tree_ctx, method_holder &methods, asIScriptContext &context, args &&...arguments) {
        auto &matching_instances = instances_by_method[type];
        scripts_operation_result result{.is_ok = true};
        for(auto &&[script_id, object]: matching_instances) {
            {
                // Assign neccessary variables
                auto &pre_lifecycle_setup = methods.at(script_id).pre_lifecycle_setup;
                auto &&setup_result = exec(context, pre_lifecycle_setup, *object, tree_ctx);
                result.merge(setup_result);
                if(!setup_result.is_ok) {
                    continue;
                }
            }
            {
                // Actually executing the method
                auto &maybe_method = method_list<type>::get(methods, script_id);
                assert(maybe_method.has_value() && "Instances by method was incorrectly modified");
                auto &&this_script_result = exec(context, maybe_method.value(), *object, std::forward<args>(arguments)...);
                result.merge(this_script_result);
            }
        }
        return result;
    }
};
} // namespace st::ags
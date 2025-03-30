module;

#include <cassert>
#include <type_traits>
#include <utility>

export module stay3.ecs:dependency;

import :ecs_registry;
import :component;

namespace st {

template<component deps, component base>
struct ecs_dependency {};

template<typename deps, typename... args>
void add_dependency(args &&...arguments, ecs_registry &reg, entity en) {
    assert(!reg.contains<deps>(en) && "Dependency already exists, consider soft dependency");
    reg.emplace<const deps>(en, std::forward<args>(arguments)...);
}

template<typename deps>
void remove_dependency(ecs_registry &reg, entity en) {
    if(reg.contains<deps>(en)) {
        reg.destroy<deps>(en);
    }
}

template<typename deps, typename base, typename... args>
void add_soft_dependency(args &&...arguments, ecs_registry &reg, entity en) {
    const auto is_deps = !reg.contains<deps>(en);
    if(!is_deps) { return; }
    reg.emplace<const ecs_dependency<deps, base>>(en);
    reg.emplace<const deps>(en, std::forward<args>(arguments)...);
}

template<typename deps, typename base>
void remove_soft_dependency(ecs_registry &reg, entity en) {
    const auto is_deps = reg.contains<ecs_dependency<deps, base>>(en);
    if(!is_deps) { return; }
    if(reg.contains<deps>(en)) {
        reg.destroy<deps>(en);
    }
}

/**
 * @brief Adds `deps` when `base` is added, and removes it when `base` is removed
 */
export template<component deps, component base, typename... args>
    requires(!std::is_same_v<std::decay_t<deps>, std::decay_t<base>> && sizeof...(args) <= 1)
void make_hard_dependency(ecs_registry &reg, args &&...arguments) {
    reg.template on<comp_event::construct, base>().template connect<&add_dependency<deps, args...>>(std::forward<args>(arguments)...);

    reg.template on<comp_event::destroy, base>().template connect<&remove_dependency<deps>>();
}

/**
 * @brief Considers `deps` a dependency if when `base` is added, `deps` does not exist
 */
export template<component deps, component base, typename... args>
    requires(!std::is_same_v<std::decay_t<deps>, std::decay_t<base>> && sizeof...(args) <= 1)
void make_soft_dependency(ecs_registry &reg, args &&...arguments) {
    reg.template on<comp_event::construct, base>().template connect<&add_soft_dependency<deps, base, args...>>(std::forward<args>(arguments)...);

    reg.template on<comp_event::destroy, base>().template connect<&remove_soft_dependency<deps, base>>();
}

} // namespace st
module;

#include <entt/entt.hpp>

export module stay3.system:signal;

export namespace st {
template<typename signature>
using signal = entt::sigh<signature>;
template<typename type>
using sink = entt::sink<type>;
} // namespace st
module;

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <format>
#include <tuple>
#include <utility>
#include <angelscript.h>
export module stay3.system.script.angelscript:register_vector;

import stay3.core;
import :engine;
import :register_type;

namespace st::ags {

template<typename type>
const char *const type_name = "?unresolved-vec-name";

template<>
const char *const type_name<float> = "float";
template<>
const char *const type_name<std::int32_t> = "int";

template<std::size_t length, typename type>
const std::string vec_name = std::format("Vec{}{}", length, *type_name<type>);

template<typename type, std::size_t...>
using dummy = type;

template<std::size_t length, typename type, std::size_t... required_param_indices>
void register_vec_single_constructor(ags_engine &engine, std::index_sequence<required_param_indices...>) {
    constexpr auto required_param_cnt = sizeof...(required_param_indices);
    const auto sig = (+[]() -> std::string {
        std::string sig{"void f("};
        for(auto i = 0; i < required_param_cnt; ++i) {
            if(i > 0) { sig += ','; }
            sig += "{0}";
        }
        sig += ')';
        return std::vformat(sig, std::make_format_args(type_name<type>));
    })();
    constexpr auto *ctor = +[](void *mem, dummy<type, required_param_indices>... args) -> void {
        std::array<type, length> components{args...};
        if constexpr(required_param_cnt < length) {
            for(auto i = required_param_cnt; i < length; ++i) {
                components[i] = type{};
            }
        }
        std::apply(
            [mem]<typename... ts>(ts... args) { new(mem) base_vec<length, type>{args...}; },
            components);
    };
    const auto res = engine->RegisterObjectBehaviour(
        vec_name<length, type>.c_str(), asBEHAVE_CONSTRUCT,
        sig.c_str(), asFUNCTION(ctor), asCALL_CDECL_OBJFIRST);
    assert(res >= 0 && "Failed to register vector constructor");
}

template<std::size_t length, typename type>
void register_vec_constructors(ags_engine &engine) {
    ([]<std::size_t... indices>(ags_engine &engine, std::index_sequence<indices...>) {
        (register_vec_single_constructor<length, type>(
             engine, std::make_index_sequence<indices>{}),
         ...);
    })(engine, std::make_index_sequence<length + 1>{});
}

template<std::size_t length, typename type>
void register_vec(ags_engine &engine) {
    const auto &name_str = vec_name<length, type>;
    const auto *name = name_str.c_str();
    using vec_type = base_vec<length, type>;
    auto additional_flag = 0;
    if(std::is_integral_v<type>) {
        additional_flag = asOBJ_APP_CLASS_ALLINTS;
    } else if(std::is_same_v<type, float>) {
        additional_flag = asOBJ_APP_CLASS_ALLFLOATS;
    } else if(std::is_same_v<type, double>) {
        additional_flag = (asOBJ_APP_CLASS_ALLFLOATS | asOBJ_APP_CLASS_ALIGN8);
    }
    auto res = engine->RegisterObjectType(
        name, sizeof(vec_type), asOBJ_VALUE | asGetTypeTraits<vec_type>() | additional_flag);
    assert(res >= 0 && "Failed to register vec");
    {
        const auto decl = std::format("{0}& opAssign(const {0} &in)", name);
        res = engine->RegisterObjectMethod(name, decl.c_str(), asMETHODPR(vec_type, operator=, (const vec_type &), vec_type &), asCALL_THISCALL);
        assert(res >= 0 && "Failed to register vec assignment");
    }
    {
        constexpr auto *dtor = +[](void *mem) -> void {
            static_cast<vec_type *>(mem)->~vec_type();
        };
        res = engine->RegisterObjectBehaviour(name, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(dtor), asCALL_CDECL_OBJLAST);
        assert(res >= 0 && "Failed to register vec destructor");
    }
    {
        register_vec_constructors<length, type>(engine);
    }
    {
        if constexpr(length >= 1) {
            const auto prop_sig = std::format("{} x", type_name<type>);
            res = engine->RegisterObjectProperty(name, prop_sig.c_str(), asOFFSET(vec_type, x));
            assert(res >= 0 && "Failed to register vec prop x");
        }
        if constexpr(length >= 2) {
            const auto prop_sig = std::format("{} y", type_name<type>);
            res = engine->RegisterObjectProperty(name, prop_sig.c_str(), asOFFSET(vec_type, y));
            assert(res >= 0 && "Failed to register vec prop y");
        }
        if constexpr(length >= 3) {
            const auto prop_sig = std::format("{} z", type_name<type>);
            res = engine->RegisterObjectProperty(name, prop_sig.c_str(), asOFFSET(vec_type, z));
            assert(res >= 0 && "Failed to register vec prop z");
        }
        if constexpr(length >= 4) {
            const auto prop_sig = std::format("{} w", type_name<type>);
            res = engine->RegisterObjectProperty(name, prop_sig.c_str(), asOFFSET(vec_type, w));
            assert(res >= 0 && "Failed to register vec prop w");
        }
        // TODO: Add unit tests
    }
}

export void register_all_vectors(ags_engine &engine) {
    register_vec<3, float>(engine);
    register_vec<2, std::int32_t>(engine);
}

} // namespace st::ags
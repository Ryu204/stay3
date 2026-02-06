module;

#include <cassert>
#include <cstdint>
#include <type_traits>
#include <angelscript.h>
export module stay3.system.script.angelscript:register_tree_context;

import stay3.ecs;
import stay3.node;
import :engine;
import :register_type;

namespace st::ags {

class node_wrapper {
    tree_context *tree;
    node::id_type id;
    friend void register_tree_context(ags_engine &engine);

    static_assert(std::is_pointer_v<std::decay_t<decltype(tree)>>, "Not non-float primitive");
    static_assert(std::is_integral_v<std::decay_t<decltype(id)>>, "Not non-float primitive");

    node &get_node() {
        assert(tree != nullptr && "Null tree");
        return tree->get_node(id);
    }

    [[nodiscard]] const node &get_node() const {
        assert(tree != nullptr && "Null tree");
        return tree->get_node(id);
    }

public:
    node_wrapper(): tree{nullptr}, id{0} {}
    node_wrapper(const node &raw, tree_context &tree): tree{&tree}, id{raw.id()} {}
};

void node_wrapper_ctor(void *mem) {
    new(mem) node_wrapper;
}

void node_wrapper_dtor(void *mem) {
    static_cast<node_wrapper *>(mem)->~node_wrapper();
}

export void register_tree_context(ags_engine &engine) {
    const auto *tree_class_name = "TreeContext";
    auto res = engine->RegisterObjectType(tree_class_name, sizeof(tree_context), asOBJ_REF | asOBJ_NOCOUNT);
    assert(res >= 0 && "Failed to register tree_context");

    const auto *node_class_name = "Node";
    res = engine->RegisterObjectType(
        node_class_name, sizeof(node_wrapper),
        asOBJ_VALUE | asGetTypeTraits<node_wrapper>()
            | asOBJ_APP_CLASS_MORE_CONSTRUCTORS | asOBJ_APP_CLASS_ALLINTS);
    assert(res >= 0 && "Failed to register node");
    res = engine->RegisterObjectBehaviour(node_class_name, asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(node_wrapper_ctor), asCALL_CDECL_OBJLAST);
    assert(res >= 0 && "Failed to register node ctor");
    res = engine->RegisterObjectBehaviour(node_class_name, asBEHAVE_DESTRUCT, "void f()", asFUNCTION(node_wrapper_dtor), asCALL_CDECL_OBJLAST);
    assert(res >= 0 && "Failed to register node dtor");
    res = engine->RegisterObjectMethod(node_class_name, "Node& opAssign(const Node &in)", asMETHODPR(node_wrapper, operator=, (const node_wrapper &), node_wrapper &), asCALL_THISCALL);
    assert(res >= 0 && "Failed to register node assign op");

    {
        constexpr auto fptr = +[](entity en, tree_context *ctx) -> node_wrapper {
            return {ctx->get_node(en), *ctx};
        };
        res = engine->RegisterObjectMethod(tree_class_name, "Node getNode(Entity en)", asFUNCTION(fptr), asCALL_CDECL_OBJLAST);
        assert(res >= 0 && "Failed to register getNode");
    }

    {
        constexpr auto fptr = +[](node_wrapper *nw) -> node_wrapper {
            return {nw->get_node().add_child(), *nw->tree};
        };
        res = engine->RegisterObjectMethod(node_class_name, "Node addChild()", asFUNCTION(fptr), asCALL_CDECL_OBJLAST);
        assert(res >= 0 && "Failed to register addChild");
    }
    {
        constexpr auto get_fptr = +[](const node_wrapper *nw) -> node_wrapper {
            return {nw->get_node().parent(), *nw->tree};
        };
        res = engine->RegisterObjectMethod(node_class_name, "Node get_parent() const property", asFUNCTION(get_fptr), asCALL_CDECL_OBJLAST);
        assert(res >= 0 && "Failed to register get_parent");
        constexpr auto set_fptr = +[](node_wrapper &parent, node_wrapper *nw) -> void {
            nw->get_node().reparent(parent.get_node());
        };
        res = engine->RegisterObjectMethod(node_class_name, "void set_parent(Node) property", asFUNCTION(set_fptr), asCALL_CDECL_OBJLAST);
        assert(res >= 0 && "Failed to register set_parent");
    }
    {
        constexpr auto fptr = +[](node_wrapper *nw) -> node::id_type {
            return nw->get_node().id();
        };
        static_assert(std::is_same_v<std::uint32_t, node::id_type>, "Host and script type mismatch");
        res = engine->RegisterObjectMethod(node_class_name, "uint32 get_id() const property", asFUNCTION(fptr), asCALL_CDECL_OBJLAST);
        assert(res >= 0 && "Failed to register get_id");
    }
}

export template<>
struct register_type<node_wrapper>: set_func_arg_object_mixin<node_wrapper> {
};
static_assert(is_registered_type<node_wrapper>);

export template<>
struct register_type<tree_context>: set_func_arg_object_mixin<tree_context> {
};
static_assert(is_registered_type<tree_context>);
} // namespace st::ags
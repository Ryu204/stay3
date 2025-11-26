module;

#include <initializer_list>

export module stay3.system.script.lua:global_fields;

export namespace st {
// NOLINTNEXTLINE(*-macro-usage)
#define STAY3_ENGINE_PREFIX "stay3_"
constexpr auto lua_field_entities_data_components = "components";
constexpr auto lua_field_component_base = "component";
constexpr auto lua_field_component_base_on_attach = "_on_attached";
constexpr auto lua_field_component_base_on_detach = "_on_detached";
constexpr auto lua_field_component_base_start = "start";
constexpr auto lua_field_component_base_update = "update";
constexpr auto lua_field_component_base_post_update = "post_update";
constexpr auto lua_field_component_base_input = "input";
constexpr auto lua_field_component_base_on_destroy = "on_destroy";
constexpr auto lua_field_component_name = "name";
constexpr auto lua_field_component_base_fields = {
    lua_field_component_base_on_attach,
    lua_field_component_base_on_detach,
    lua_field_component_base_start,
    lua_field_component_base_update,
    lua_field_component_base_post_update,
    lua_field_component_base_input,
    lua_field_component_base_on_destroy,
    "new",
};
#undef STAY3_ENGINE_PREFIX
} // namespace st
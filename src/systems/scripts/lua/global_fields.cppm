module;

#include <initializer_list>

export module stay3.system.script.lua:global_fields;

export namespace st {
// NOLINTNEXTLINE(*-macro-usage)
#define STAY3_ENGINE_PREFIX "stay3_"
constexpr auto lua_field_entities_data_components = "components";
constexpr auto lua_field_component_base = "component";
constexpr auto lua_field_component_base_on_attach = "_on_attached";
constexpr auto lua_field_component_name = "name";
constexpr auto lua_field_component_base_fields = {
    lua_field_component_base_on_attach,
    "new",
};
#undef STAY3_ENGINE_PREFIX
} // namespace st
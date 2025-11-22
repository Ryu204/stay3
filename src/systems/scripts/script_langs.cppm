module;

#include <cstdint>
#include <string_view>

export module stay3.system.script:script_langs;

export namespace st {
enum class script_lang : std::uint8_t {
    lua,
};
constexpr std::string_view script_lang_name(script_lang lang) {
    switch(lang) {
    case script_lang::lua:
        return "lua";
    default:
        return "Unimplemented name";
    }
}
} // namespace st
module;

#include <cassert>
#include <cstdint>
#include <filesystem>
#include <queue>
#include <variant>
#include <webgpu/webgpu_cpp.h>

export module stay3.graphics:texture;

import stay3.core;
import stay3.ecs;

export namespace st {

class texture_2d {
public:
    enum class format : std::uint8_t {
        rgba8u_norm,
    };
    struct command_write {
        component_ref<texture_2d> target;
        std::vector<std::uint8_t> data;
        vec2u origin;
    };
    struct command_copy {
        component_ref<texture_2d> target;
        component_ref<texture_2d> source;
        vec2u offset;
        vec2u size;
    };
    struct command_load {
        entity target;
        std::optional<format> preferred_format = std::nullopt;
        std::filesystem::path filename;
    };
    using commands = std::queue<std::variant<command_write, command_copy, command_load>>;

    static wgpu::TextureFormat from_enum(format fm) {
        switch(fm) {
        case format::rgba8u_norm:
            return wgpu::TextureFormat::RGBA8Unorm;
        default:
            assert(false && "Unimplemented");
        }
    }
    static unsigned int format_to_channel_count(format fm) {
        switch(fm) {
        case format::rgba8u_norm:
            return 4;
        default:
            assert(false && "Unimplemented");
        }
    }
    texture_2d(format fm = format::rgba8u_norm, const vec2u &size = {256, 256})
        : m_format{fm}, m_size{size} {
    }

    [[nodiscard]] const vec2u &size() const {
        return m_size;
    }
    [[nodiscard]] auto channel_count() const {
        return format_to_channel_count(m_format);
    }
    [[nodiscard]] auto texture_format() const {
        return m_format;
    }

private:
    format m_format{format::rgba8u_norm};
    vec2u m_size;
};

} // namespace st
module;

#include <bit>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <type_traits>
#include <variant>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#include <webgpu/webgpu_cpp.h>

export module stay3.graphics:material;

import stay3.core;
import stay3.ecs;

export namespace st {

enum class texture_load_result : std::uint8_t {
    success,
    file_not_found,
    error,
};

class texture_2d_data {
    // NOLINTNEXTLINE(*-avoid-c-arrays)
    using stbi_loaded_data = std::unique_ptr<std::uint8_t[], decltype(&stbi_image_free)>;
    using uploaded_data = std::vector<std::uint8_t>;
    using data = std::variant<std::monostate, stbi_loaded_data, uploaded_data>;

public:
    enum class format : std::uint8_t {
        rgba8u_norm,
    };
    static wgpu::TextureFormat from_enum(format fm) {
        switch(fm) {
        case format::rgba8u_norm:
            return wgpu::TextureFormat::RGBA8Unorm;
        default:
            assert(false && "Unimplemented");
        }
    }
    [[nodiscard]] const std::uint8_t *data_ptr() const {
        return std::visit(
            visit_helper{
                [](const std::monostate &) -> const std::uint8_t * {
                    return nullptr;
                },
                [](const stbi_loaded_data &ptr) -> const std::uint8_t * {
                    return ptr.get();
                },
                [](const uploaded_data &vec) -> const std::uint8_t * {
                    return vec.data();
                },
            },
            raw_data);
    }

    texture_2d_data(const std::filesystem::path &texture_file) {
        const auto load_status = load_from_file(texture_file);
        switch(load_status) {
        case texture_load_result::success:
            break;
        case texture_load_result::file_not_found:
            log::warn("Texture load failed, file ", texture_file.string(), " not found");
            break;
        case texture_load_result::error:
            log::warn("Texture load failed with file ", texture_file.string());
            break;
        }
    }
    [[nodiscard]] texture_load_result load_from_file(const std::filesystem::path &path) {
        if(!std::filesystem::exists(path) || std::filesystem::is_directory(path)) {
            return texture_load_result::file_not_found;
        }
        vec2i size;
        int file_channel_count{};
        constexpr auto desired_channel_count = 4;
        auto *pixels = stbi_load(path.string().c_str(), &size.x, &size.y, &file_channel_count, desired_channel_count);
        static_assert(std::is_same_v<decltype(pixels), stbi_loaded_data::pointer>);

        if(pixels == nullptr || size.x <= 0 || size.y <= 0) {
            return texture_load_result::error;
        }

        this->raw_data = stbi_loaded_data{pixels, &stbi_image_free};
        this->m_format = format::rgba8u_norm;
        this->m_size = size;
        this->m_channel_count = desired_channel_count;
        return texture_load_result::success;
    }
    texture_2d_data(uploaded_data &&data, format fm, const vec2u &size, unsigned int channel_count)
        : raw_data{std::move(data)}, m_format{fm}, m_size{size}, m_channel_count{channel_count} {
        assert(
            std::get<uploaded_data>(raw_data).size() == static_cast<std::size_t>(channel_count * size.x) * size.y
            && "Invalid data size");
    }

    [[nodiscard]] const vec2u &size() const {
        return m_size;
    }
    [[nodiscard]] auto channel_count() const {
        return m_channel_count;
    }
    [[nodiscard]] auto texture_format() const {
        return m_format;
    }

private:
    data raw_data;
    format m_format{format::rgba8u_norm};
    vec2u m_size;
    unsigned int m_channel_count{};
};

struct material_data {
    entity texture_holder;
};

} // namespace st
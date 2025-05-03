module;

#include <cassert>
#include <filesystem>
#include <memory>
#include <mutex>
#include <ft2build.h>
#include FT_FREETYPE_H

export module stay3.graphics:font;

import stay3.core;
import stay3.node;
import stay3.ecs;
import :material;
import :error;

namespace st {

class freetype_context {
public:
    freetype_context() {
        if(FT_Init_FreeType(&m_library) != FT_Err_Ok) {
            throw graphics_error{"Failed to init FreeType"};
        }
    }
    ~freetype_context() {
        if(FT_Done_FreeType(m_library) != FT_Err_Ok) {
            log::error("Failed to cleanup FreeType");
        }
    }

    freetype_context(freetype_context &&) noexcept = delete;
    freetype_context(const freetype_context &) = delete;
    freetype_context &operator=(freetype_context &&) noexcept = delete;
    freetype_context &operator=(const freetype_context &) = delete;

    [[nodiscard]] auto library() const {
        return m_library;
    }

private:
    FT_Library m_library{};
};

class freetype_context_user {
public:
    freetype_context_user()
        : m_context{[]() {
              static std::mutex mutex;
              static std::weak_ptr<freetype_context> weak_context;

              const std::lock_guard lock{mutex};
              auto context = weak_context.lock();
              if(!context) {
                  context = std::make_shared<freetype_context>();
                  weak_context = context;
              }
              return context;
          }()} {}

protected:
    [[nodiscard]] auto library() const {
        return m_context->library();
    }

private:
    std::shared_ptr<freetype_context> m_context;
};

export struct glyph_metrics {
    unsigned int width{};
    unsigned int height{};
};

export class font: private freetype_context_user {
public:
    using size_type = std::uint32_t;
    using glyph_index = FT_UInt;
    using character = FT_ULong;

    font(const std::filesystem::path &font_file) {
        if(!std::filesystem::exists(font_file) || std::filesystem::is_directory(font_file)) {
            log::warn("Failed to load font: \"", font_file, "\": Invalid file");
        }
        const auto new_face_status = FT_New_Face(library(), font_file.string().c_str(), 0, &m_font_face);
        switch(new_face_status) {
        case FT_Err_Ok:
            break;
        case FT_Err_Unknown_File_Format:
            log::warn("Unknown font format: \"", font_file, '"');
            break;
        default:
            log::warn("Failed to load font: \"", font_file, "\": Unknown error");
            break;
        }
    }
    ~font() {
        if(FT_Done_Face(m_font_face) != FT_Err_Ok) {
            log::error("Failed to cleanup font");
        }
    }
    font(font &&) noexcept = delete;
    font(const font &) = delete;
    font &operator=(font &&) noexcept = delete;
    font &operator=(const font &) = delete;

    [[nodiscard]] glyph_index character_to_index(character ch) const {
        return FT_Get_Char_Index(m_font_face, ch);
    }

    /**
     * @brief Set internal font size. User should not need this.
     */
    [[nodiscard]] bool set_size(size_type size) {
        assert(size > 0);
        if(size == m_current_size) { return true; }
        switch(FT_Set_Pixel_Sizes(m_font_face, 0, size)) {
        case FT_Err_Ok:
            m_current_size = size;
            return true;
        case FT_Err_Invalid_Pixel_Size:
            // https://github.com/SFML/SFML/blob/0b26f7d435cb8da19af70c4a331cf40c6d78c905/src/SFML/Graphics/Font.cpp#L716
            if(!FT_IS_SCALABLE(m_font_face)) {
                log::error("Failed to set bitmap font size to ", size);
                log::info("Available font sizes are: ");
                for(auto i = 0; i < m_font_face->num_fixed_sizes; ++i) {
                    // NOLINTNEXTLINE(*-pointer-arithmetic)
                    const auto size = (m_font_face->available_sizes[i].y_ppem + 32) >> 6;
                    log::info("\t", size);
                }
                break;
            }
        default:
            log::error("Failed to set bitmap font size to ", size);
        }
        return false;
    }

    [[nodiscard]] struct glyph_metrics glyph_metrics(glyph_index index) const {
        FT_Load_Glyph(m_font_face, index, FT_LOAD_DEFAULT);
        const auto &metrics = m_font_face->glyph->metrics;
        return {
            .width = static_cast<unsigned int>(metrics.width),
            .height = static_cast<unsigned int>(metrics.height),
        };
    }

private:
    FT_Face m_font_face{};
    size_type m_current_size{};
};

} // namespace st
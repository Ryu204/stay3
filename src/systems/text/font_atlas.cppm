module;

#include <cassert>
#include <optional>
#include <unordered_map>
#include <vector>
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
export module stay3.system.text:font_atlas;

import stay3.graphics.core;
import stay3.graphics.text;
import stay3.ecs;
import stay3.core;

export namespace st {
struct glyph_info {
    rect<unsigned int> texture_rect;
    glyph_metrics metrics;
};
class rect_packer {
public:
    constexpr rect_packer(unsigned int padding)
        : padding{padding} {};
    void set_size(const vec2u &size) {
        assert(size.x > 0 && size.y > 0);
        nodes.resize(size.x);
        stbrp_init_target(&context, static_cast<int>(size.x), static_cast<int>(size.y), nodes.data(), static_cast<int>(nodes.size()));
        current_size = size;
    }
    [[nodiscard]] const vec2u &size() const {
        return current_size;
    }
    [[nodiscard]] std::optional<rect<unsigned int>> pack(const vec2u &new_rect) {
        stbrp_rect stbrect{
            .w = static_cast<int>((padding * 2) + new_rect.x),
            .h = static_cast<int>((padding * 2) + new_rect.y),
            .was_packed = 0,
        };
        const auto result = stbrp_pack_rects(
            &context,
            &stbrect,
            1);
        if(result != 1) {
            return std::nullopt;
        }
        return rect<unsigned int>{
            .position = {stbrect.x + padding, stbrect.y + padding},
            .size = new_rect,
        };
    }

private:
    unsigned int padding;
    vec2u current_size;
    std::vector<stbrp_node> nodes;
    stbrp_context context;
};

struct font_atlas {
    component_ref<texture_2d> texture_holder;
    std::unordered_map<font::glyph_index, std::size_t> available_glyphs;
    std::vector<glyph_info> glyph_data;
    rect_packer bin_packer{2};
};
} // namespace st
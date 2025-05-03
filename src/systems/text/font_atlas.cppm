module;

#include <unordered_map>
#include <vector>
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb/stb_rect_pack.h>
export module stay3.system.text:font_atlas;

import stay3.graphics;
import stay3.ecs;
import stay3.core;

export namespace st {
struct glyph_info {
    rect<unsigned int> texture_rect;
    glyph_metrics metrics;
};
class rect_packer {
public:
    void set_size(const vec2u &size) {
        nodes.resize(size.x);
        packed_count = 0;
        stbrp_init_target(&context, static_cast<int>(size.x), static_cast<int>(size.y), nodes.data(), static_cast<int>(nodes.size()));
        current_size = size;
    }
    [[nodiscard]] const vec2u &size() const {
        return current_size;
    }
    [[nodiscard]] bool pack_all() {
        if(packed_count == rects.size()) { return true; }
        const auto result = stbrp_pack_rects(
            &context,
            (rects.begin() + static_cast<std::ptrdiff_t>(packed_count)).base(),
            static_cast<int>(rects.size() - packed_count));
        if(result == 1) {
            packed_count = rects.size();
            return true;
        }
        return false;
    }
    void add(const vec2u &new_rect) {
        rects.emplace_back(stbrp_rect{
            .w = static_cast<int>(new_rect.x),
            .h = static_cast<int>(new_rect.y),
            .was_packed = 0,
        });
    }
    [[nodiscard]] rect<unsigned int> rect(std::size_t index) const {
        return {
            .position = {rects[index].x, rects[index].y},
            .size = {rects[index].w, rects[index].h},
        };
    }

private:
    vec2u current_size;
    std::vector<stbrp_node> nodes;
    stbrp_context context;
    std::vector<stbrp_rect> rects;
    std::size_t packed_count{0};
};
struct font_atlas {
    component_ref<texture_2d> texture_holder;
    std::unordered_map<font::glyph_index, std::size_t> available_glyphs;
    std::vector<glyph_info> glyph_data;
    rect_packer bin_packer;
};
} // namespace st
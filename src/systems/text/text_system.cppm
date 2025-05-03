module;

#include <cassert>
#include <cstdint>
#include <vector>

export module stay3.system.text;

import stay3.node;
import stay3.ecs;
import stay3.graphics;

import :font_state;
import :font_atlas;
import :text_state;

export namespace st {
class text_system {
public:
    static void start(tree_context &ctx) {
        auto &reg = ctx.ecs();

        make_hard_dependency<font_state, font>(reg);
        destroy_atlases_with_font(reg);
        make_texture_atlas_dependency(ctx);
        track_text_changes(reg);
        create_mesh_with_text(reg);
    }
    static void render(tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto en: reg.view<text_data_changed>()) {
            build_text_geometry(ctx, reg, en);
        }
        reg.destroy_all<text_data_changed>();
    }

private:
    static void make_texture_atlas_dependency(tree_context& ctx) {
        auto& reg = ctx.ecs();
        static constexpr vec2u default_texture_size{128u};
        static constexpr auto default_format = texture_2d::format::rgba8u_norm;
        reg.on<comp_event::construct, font_atlas>().connect<+[](tree_context& ctx, ecs_registry &reg, entity en) {
            reg.emplace<texture_2d>(
                en,
                default_format, // TODO: use 1 channel instead of rgba
                default_texture_size
            );
            ctx.vars().get<texture_2d::commands>().push(texture_2d::command_write{
                .target = en,
                .data = std::vector<std::uint8_t>(texture_2d::format_to_channel_count(default_format) * default_texture_size.x * default_texture_size.y, 0),
            });
        }>(ctx);
        reg.on<comp_event::destroy, font_atlas>().connect<&ecs_registry::destroy_if_exist<texture_2d>>();
    }
    static void destroy_atlases_with_font(ecs_registry &reg) {
        reg.on<comp_event::destroy, font_state>().connect<+[](ecs_registry &reg, entity en) {
            auto state = reg.get<font_state>(en);
            for(auto &&[holder, comp_ref]: state->atlas_holders) {
                reg.destroy_if_exist(comp_ref.entity());
            }
        }>();
    }
    static void track_text_changes(ecs_registry &reg) {
        reg.on<comp_event::construct, text>().connect<&ecs_registry::emplace_if_not_exist<text_data_changed>>();
        reg.on<comp_event::update, text>().connect<&ecs_registry::emplace_if_not_exist<text_data_changed>>();
        reg.on<comp_event::destroy, text>().connect<&ecs_registry::destroy_if_exist<text_data_changed>>();
    }
    static void create_mesh_with_text(ecs_registry &reg) {
        reg.on<comp_event::construct, text>().connect<+[](ecs_registry &reg, entity en) {
            assert(!reg.contains<mesh_data>(en) && "Text entity cannot have mesh_data beforehand");
            reg.emplace<mesh_data>(en);
            assert(!reg.contains<rendered_mesh>(en) && "Text entity cannot have rendered_mesh beforehand");
            auto text_font = reg.get<text>(en)->font;
            reg.emplace<rendered_mesh>(en, rendered_mesh{.mesh = en});
        }>();
    }
    static void build_text_geometry(tree_context &ctx, ecs_registry &reg, entity text_entity) {
        auto [txt, mesh] = reg.get<text, mut<mesh_data>>(text_entity);
        auto [fnt_data, fnt_state] = reg.get<mut<font>, font_state>(txt->font.entity());
        if(!fnt_state->atlas_holders.contains(txt->size)) {
            create_font_atlas(ctx, txt->font.entity());
        }
        auto fnt_atlas = fnt_state->atlas_holders.at(txt->size).get_mut(reg);
        const auto max_character_count = txt->content.length();
        auto &vertices = mesh->vertices;
        vertices.reserve(max_character_count * 4);
        for(auto character: txt->content) {
            const auto index = fnt_data->character_to_index(character);
            if(!fnt_atlas->available_glyphs.contains(index)) {
                add_glyph_to_atlas(*fnt_data, txt->size, *fnt_atlas, *fnt_atlas->texture_holder.get_mut(reg), index);
            }
            const auto &glyph = fnt_atlas->available_glyphs.at(index);
        }
    }
    static void create_font_atlas(tree_context &ctx, entity font_holder) {
        auto &node = ctx.get_node(font_holder);
        const auto atlas_holder = node.entities().create();
        ctx.ecs().emplace<font_atlas>(atlas_holder);
    }
    static void add_glyph_to_atlas(font &fnt, font::size_type size, font_atlas &atlas, texture_2d &texture, font::glyph_index index) {
        if(!fnt.set_size(size)) {
            throw graphics_error{"Failed to set font size"};
        }
        const auto glyph = fnt.glyph_metrics(index);
        atlas.bin_packer.add({glyph.width, glyph.height});
        bool is_resized{false};
        while(!atlas.bin_packer.pack_all()) {
            constexpr vec2u max_texture_size{1024u};
            constexpr auto scale = 2;
            const auto can_scale = atlas.bin_packer.size().x * scale <= max_texture_size.x
                                   && atlas.bin_packer.size().y * scale <= max_texture_size.y;
            if(!can_scale) {
                throw graphics_error{"Font atlas could not fit all glyphs inside texture"};
            }
            is_resized = true;
        }
        if(is_resized) {
            // Create copy of texture, resize original texture
            // Copy existing glyphs into original texture
            // ...
        }
        // Paste content of new glyph into the texture
        // ...
        auto old_glyph_count = atlas.glyph_data.size();
        atlas.glyph_data.push_back(glyph_info{
            .texture_rect = atlas.bin_packer.rect(old_glyph_count),
            .metrics = glyph,
        });
        atlas.available_glyphs.emplace(index, old_glyph_count);
    }
};
} // namespace st

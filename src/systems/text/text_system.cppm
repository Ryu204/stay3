module;

#include <cassert>
#include <cstdint>
#include <vector>

export module stay3.system.text;

import stay3.node;
import stay3.ecs;
import stay3.graphics.core;
import stay3.graphics.text;

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
        make_texture_material_atlas_dependency(ctx);
        track_text_changes(reg);
        create_mesh_with_text(reg);
    }
    static void render(tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto en: reg.view<text_data_changed>()) {
            build_text_geometry(ctx, reg, en);
            if(!reg.contains<rendered_mesh>(en)) {
                initialize_rendered_mesh(reg, en);
            }
        }
        reg.destroy_all<text_data_changed>();
    }

private:
    static constexpr vec2u default_texture_size{512u};
    static constexpr float pixels_per_unit{50.F};

    static void make_texture_material_atlas_dependency(tree_context &ctx) {
        auto &reg = ctx.ecs();
        static constexpr auto default_format = texture_2d::format::rgba8unorm;
        reg.on<comp_event::construct, font_atlas>().connect<+[](tree_context &ctx, ecs_registry &reg, entity en) {
            reg.emplace<texture_2d>(
                en,
                default_format,
                default_texture_size);
            ctx.vars().get<texture_2d::commands>().emplace(texture_2d::command_write{
                .target = en,
                .data = std::vector<std::uint8_t>(
                    static_cast<std::size_t>(texture_2d::format_to_channel_count(default_format))
                        * default_texture_size.x * default_texture_size.y,
                    0),
            });
            reg.emplace<material>(en, material{.texture = en});
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
        }>();
    }

    static void initialize_rendered_mesh(ecs_registry &reg, entity en) {
        auto txt = reg.get<text>(en);
        auto text_font = txt->font;
        auto atlas_en = reg.get<font_state>(text_font.entity())->atlas_holders.at(txt->size).entity();
        reg.emplace<rendered_mesh>(
            en,
            rendered_mesh{
                .mesh = en,
                .mat = atlas_en,
                .transparency = true,
            });
    }

    static void build_text_geometry(tree_context &ctx, ecs_registry &reg, entity text_entity) {
        auto [txt, mesh] = reg.get<text, mut<mesh_data>>(text_entity);
        auto [fnt_data, fnt_state] = reg.get<mut<font>, mut<font_state>>(txt->font.entity());
        if(!fnt_state->atlas_holders.contains(txt->size)) {
            auto new_atlas_holder = create_font_atlas(ctx, txt->font.entity());
            fnt_state->atlas_holders.emplace(txt->size, new_atlas_holder);
        }
        auto atlas_holder = fnt_state->atlas_holders.at(txt->size);
        auto fnt_atlas = atlas_holder.get_mut(reg);
        const auto max_character_count = txt->content.length();
        auto &vertices = mesh->vertices;
        auto &indices = mesh->maybe_indices;
        vertices.clear();
        vertices.reserve(max_character_count * 4);
        indices.emplace();
        constexpr auto indices_per_quad = 6;
        indices->reserve(max_character_count * indices_per_quad);
        auto pen_x = 0.F;
        const auto &texture_size = fnt_atlas->texture_holder.get(reg)->size();
        if(!fnt_data->set_size(txt->size)) {
            throw graphics_error{"Failed to set font size"};
        }
        const auto whitespace_advance = fnt_data->whitespace_advance();
        // Iterate over codepoints
        for(auto character: txt->content) {
            switch(character) {
            case ' ':
                pen_x += static_cast<float>(whitespace_advance) / 64.F;
                continue;
            case '\t':
                pen_x += static_cast<float>(whitespace_advance) / 16.F;
                continue;
            default:
                break;
            }

            const auto index = fnt_data->character_to_index(character);
            if(!fnt_atlas->available_glyphs.contains(index)) {
                add_glyph_to_atlas(ctx, fnt_atlas->texture_holder, *fnt_data, *fnt_atlas, index);
            }
            const auto &glyph = fnt_atlas->glyph_data[fnt_atlas->available_glyphs.at(index)];
            append_geometry(*mesh, pen_x, glyph.metrics, glyph.texture_rect, texture_size);
            pen_x += static_cast<float>(glyph.metrics.advance) / 64.F;
        }
    }
    static entity create_font_atlas(tree_context &ctx, entity font_holder) {
        auto &node = ctx.get_node(font_holder);
        const auto atlas_holder = node.entities().create();
        auto &reg = ctx.ecs();
        auto atlas = reg.emplace<mut<font_atlas>>(atlas_holder, font_atlas{.texture_holder = atlas_holder});
        atlas->bin_packer.set_size(default_texture_size);
        return atlas_holder;
    }
    static void add_glyph_to_atlas(tree_context &ctx, component_ref<texture_2d> texture, font &fnt, font_atlas &atlas, font::glyph_index index) {
        auto glyph_data = fnt.load_glyph(index);
        const auto new_rect = atlas.bin_packer.pack({glyph_data.metrics.width, glyph_data.metrics.height});
        if(!new_rect.has_value()) {
            throw graphics_error{"Font atlas could not fit all glyphs inside texture"};
        }
        ctx.vars().get<texture_2d::commands>().emplace(texture_2d::command_write{
            .target = texture.entity(),
            .data = std::move(glyph_data.bitmap),
            .origin = new_rect->position,
            .size = new_rect->size,
        });
        auto old_glyph_count = atlas.glyph_data.size();
        atlas.glyph_data.emplace_back(glyph_info{
            .texture_rect = new_rect.value(),
            .metrics = glyph_data.metrics,
        });
        atlas.available_glyphs.emplace(index, old_glyph_count);
    }
    static void append_geometry(mesh_data &data, float pen_x, const glyph_metrics &metrics, const rect<unsigned int> &texture_rect, const vec2u &texture_size) {
        auto &indices = data.maybe_indices.value();
        auto &vertices = data.vertices;
        unsigned int first_index = vertices.size();
        indices.insert(indices.end(), {first_index, first_index + 2, first_index + 1});
        indices.insert(indices.end(), {first_index, first_index + 3, first_index + 2});
        vec3f top_left = vec3f{pen_x + static_cast<float>(metrics.bearing.x), static_cast<float>(metrics.bearing.y), 0.F} / pixels_per_unit;
        vertices.emplace_back(vertex_attributes{
            .position = top_left,
            .uv = vec2f{texture_rect.position} / vec2f{texture_size},
        });
        vertices.emplace_back(vertex_attributes{
            .position = top_left + static_cast<float>(metrics.width) * vec_right / pixels_per_unit,
            .uv = vec2f{texture_rect.position.x + texture_rect.size.x, texture_rect.position.y} / vec2f{texture_size},
        });
        vertices.emplace_back(vertex_attributes{
            .position = top_left + vec3f{metrics.width, -static_cast<float>(metrics.height), 0.F} / pixels_per_unit,
            .uv = vec2f{texture_rect.position + texture_rect.size} / vec2f{texture_size},
        });
        vertices.emplace_back(vertex_attributes{
            .position = top_left + static_cast<float>(metrics.height) * vec_down / pixels_per_unit,
            .uv = vec2f{texture_rect.position.x, texture_rect.position.y + texture_rect.size.y} / vec2f{texture_size},
        });
        assert(vertices.back().position.x < 100000);
    }
};
} // namespace st

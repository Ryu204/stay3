module;

#include <cassert>
#include <utility>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:material_subsystem;

import stay3.node;
import stay3.ecs;
import stay3.core;
import :init_result;
import :material;
import :bind_group_layouts;

namespace st {

struct material_state_init_pending {};
struct material_state_update_pending {};

export class material_subsystem {
public:
    void start(tree_context &tree_ctx, init_result &graphics_context, render_config &config, wgpu::BindGroupLayout material_layout) {
        m_context = &graphics_context;
        m_config = &config;
        m_material_layout = std::move(material_layout);
        setup_signals(tree_ctx);
        create_default_sampler_entity(tree_ctx);
    }

    void process_pending_materials(tree_context &ctx) {
        auto &reg = ctx.ecs();
        for(auto en: reg.view<material_state_update_pending>()) {
            update_material_state(reg, en);
        }
        reg.destroy_all<material_state_update_pending>();
    }

private:
    static void setup_signals(tree_context &ctx) {
        auto &reg = ctx.ecs();
        make_hard_dependency<material_state, material>(reg);
        reg.on<comp_event::construct, material>().connect<&ecs_registry::emplace<material_state_update_pending>>();
        reg.on<comp_event::update, material>().connect<&ecs_registry::emplace_if_not_exist<material_state_update_pending>>();
        reg.on<comp_event::destroy, material>().connect<&ecs_registry::destroy_if_exist<material_state_update_pending>>();
    }

    void update_material_state(ecs_registry &reg, entity en) const {
        auto [state, data] = reg.get<mut<material_state>, material>(en);

        wgpu::TextureView texture_view;
        {
            if(data->texture.is_null()) {
                texture_view = reg.get<texture_2d_state>(*reg.view<default_texture_tag>().begin())->view;
            } else {
                texture_view = reg.get<texture_2d_state>(data->texture.entity())->view;
            }
        }
        const auto sampler_comp = reg.get<sampler>(*reg.view<default_sampler_tag>().begin());
        const std::array<wgpu::BindGroupEntry, bind_group_layouts_data::material::binding_count> entries{
            wgpu::BindGroupEntry{
                .binding = bind_group_layouts_data::material::texture::binding,
                .textureView = texture_view,
            },
            wgpu::BindGroupEntry{
                .binding = bind_group_layouts_data::material::sampler::binding,
                .sampler = sampler_comp->sampler,
            },
        };
        const wgpu::BindGroupDescriptor bind_group_desc{
            .layout = m_material_layout,
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        state->material_bind_group = m_context->device.CreateBindGroup(&bind_group_desc);
    }

    void create_default_sampler_entity(tree_context &ctx) const {
        auto &reg = ctx.ecs();
        auto en = ctx.root().entities().create();
        const wgpu::SamplerDescriptor desc{
            .addressModeU = wgpu::AddressMode::ClampToEdge,
            .addressModeV = wgpu::AddressMode::ClampToEdge,
            .addressModeW = wgpu::AddressMode::ClampToEdge,
            .magFilter = render_config::from_enum(m_config->filter),
            .minFilter = render_config::from_enum(m_config->filter),
            .mipmapFilter = wgpu::MipmapFilterMode::Linear, // Currently mip map is not used
            .lodMinClamp = 0.F,
            .lodMaxClamp = 1.F,
            .maxAnisotropy = 1,
        };
        reg.emplace<sampler>(en, m_context->device.CreateSampler(&desc));
        reg.emplace<default_sampler_tag>(en);
    }

    init_result *m_context{};
    render_config *m_config{};
    wgpu::BindGroupLayout m_material_layout;
};
} // namespace st
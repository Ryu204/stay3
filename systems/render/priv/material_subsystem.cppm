module;

#include <cassert>
#include <utility>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render.priv:material_subsystem;

import stay3.node;
import stay3.ecs;
import stay3.core;
import :init_result;
import :material;
import :bind_group_layouts;

namespace st {

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

    [[nodiscard]] wgpu::Buffer create_properties_buffer() const {
        wgpu::BufferDescriptor buffer_desc{
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(bind_group_layouts_data::material::properties::type),
            .mappedAtCreation = false,
        };
        return m_context->device.CreateBuffer(&buffer_desc);
    }
    void update_material_state(ecs_registry &reg, entity en) const {
        auto [state, data] = reg.get<mut<material_state>, material>(en);
        const auto texture_entity = data->texture.is_null()
                                        ? *reg.view<default_texture_tag>().begin()
                                        : data->texture.entity();
        const auto sampler_entity = *reg.view<default_sampler_tag>().begin();
        const auto &texture_view = reg.get<texture_2d_state>(texture_entity)->view;
        const auto &wgpu_sampler = reg.get<sampler>(sampler_entity)->sampler;
        // Create data buffer if it does not exist
        if(!state->properties_buffer) {
            state->properties_buffer = create_properties_buffer();
        }
        // Recreate bind group if needed
        if(state->texture_entity != texture_entity || state->sampler_entity != sampler_entity) {
            state->material_bind_group = create_bind_group(texture_view, wgpu_sampler, state->properties_buffer);
            state->texture_entity = texture_entity;
            state->sampler_entity = sampler_entity;
        }
        // Upload actual data
        {
            static_assert(sizeof(material_uniform) % 4 == 0, "Not a multiple of 4");
            const material_uniform upload_data{
                .color = data->color,
            };
            m_context->queue.WriteBuffer(state->properties_buffer, 0, &upload_data, sizeof(upload_data));
        }
    }

    [[nodiscard]] wgpu::BindGroup create_bind_group(const wgpu::TextureView &texture_view, const wgpu::Sampler &sampler, const wgpu::Buffer &properties_buffer) const {
        const std::array<wgpu::BindGroupEntry, bind_group_layouts_data::material::binding_count> entries{
            wgpu::BindGroupEntry{
                .binding = bind_group_layouts_data::material::texture::binding,
                .textureView = texture_view,
            },
            wgpu::BindGroupEntry{
                .binding = bind_group_layouts_data::material::sampler::binding,
                .sampler = sampler,
            },
            wgpu::BindGroupEntry{
                .binding = bind_group_layouts_data::material::properties::binding,
                .buffer = properties_buffer,
            },
        };
        const wgpu::BindGroupDescriptor bind_group_desc{
            .layout = m_material_layout,
            .entryCount = entries.size(),
            .entries = entries.data(),
        };
        return m_context->device.CreateBindGroup(&bind_group_desc);
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
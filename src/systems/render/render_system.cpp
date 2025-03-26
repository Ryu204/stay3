module;

#include <algorithm>
#include <filesystem>
#include <webgpu/webgpu_cpp.h>

module stay3.system.render;

import stay3.node;
import stay3.ecs;
import stay3.core;
import stay3.system.runtime_info;
import stay3.system.transform;

import :init_result;
import :pipeline;
import :render_pass;
import :material;
import :components;

namespace st {

render_system::render_system(const vec2u &surface_size, std::filesystem::path shader_path, const render_config &config)
    : m_config{config}, m_surface_size{surface_size}, m_shader_path{std::move(shader_path)} {}

void render_system::start(tree_context &ctx) {
    auto &window = ctx.ecs().get_context<runtime_info>().window();
    m_global = create_and_config(window, m_config, m_surface_size);
    const texture_formats formats{
        .surface = m_global.surface_format,
        .depth = wgpu::TextureFormat::Depth24Plus,
    };
    m_depth_texture = create_depth_texture_view(m_global.device, m_surface_size, formats.depth);
    m_bind_group_layouts = bind_group_layouts{m_global.device};
    m_pipeline = create_pipeline(m_global.device, formats, m_shader_path, *m_bind_group_layouts);
    setup_signals(ctx.ecs());
}

void render_system::render(tree_context &ctx) {
    auto &reg = ctx.ecs();
    vec4f clear_color;
    // Find main camera
    {
        auto cameras = reg.each<const camera, const main_camera, const global_transform>();
        assert(cameras.begin() != cameras.end() && "No main camera found");
        auto [unused_en, cam, unused_tag, tf] = *cameras.begin();
        clear_color = cam->clear_color;
        const auto camera_view_projection = perspective(cam->fov, cam->ratio, cam->near, cam->far) * tf->get().inv_matrix();
        update_all_object_uniforms(reg, camera_view_projection);
    }

    // Draw commands
    const auto &&[unused, encoder, render_pass_encoder] = create_render_pass(m_global.device, m_global.surface, m_depth_texture.view, clear_color);
    render_pass_encoder.SetPipeline(m_pipeline);

    entity material_entity;
    for(auto &&[unused, data, state]: reg.each<const rendered_mesh, const rendered_mesh_state>()) {
        if(data->material_holder != material_entity) {
            material_entity = data->material_holder;
            const auto material_bind_group = reg.get_components<const material_state>(material_entity)->material_bind_group;
            render_pass_encoder.SetBindGroup(bind_group_layouts_data::material::group, material_bind_group);
        }
        auto [geometry_data, geometry_state] = reg.get_components<const mesh_data, const mesh_state>(data->mesh_holder);
        auto material_state = reg.get_components<const mesh_data, const mesh_state>(data->mesh_holder);
        render_pass_encoder.SetVertexBuffer(0, geometry_state->vertex_buffer);
        render_pass_encoder.SetBindGroup(bind_group_layouts_data::object::group, state->object_bind_group);
        if(geometry_state->index_buffer) {
            render_pass_encoder.SetIndexBuffer(geometry_state->index_buffer, wgpu::IndexFormat::Uint32);
            render_pass_encoder.DrawIndexed(geometry_data->maybe_indices->size(), 1, 0, 0, 0);
        } else {
            render_pass_encoder.Draw(geometry_data->vertices.size(), 1, 0, 0);
        }
    }
    render_pass_encoder.End();

    // Submit the command buffer
    const auto cmd_buffer = encoder.Finish();
    m_global.queue.Submit(1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
    m_global.surface.Present();
#endif
}

void render_system::cleanup(tree_context &) const {
    m_global.surface.Unconfigure();
}

void render_system::update_all_object_uniforms(ecs_registry &reg, const mat4f &camera_view_projection) {
    assert(std::ranges::all_of(reg.each<const rendered_mesh_state>(), [&reg](const auto &tuple) {
               return reg.has_components<global_transform>(std::get<0>(tuple));
           })
           && "rendered_mesh_state without global_transform");
    for(auto &&[unused, global_tf, state]: reg.each<const global_transform, const rendered_mesh_state>()) {
        const auto &mat = global_tf->get().matrix();
        static_assert(std::is_same_v<std::decay_t<decltype(mat)>, mat4f>);
        const mat4f mvp_matrix_uniform = camera_view_projection * global_tf->get().matrix();
        static_assert(sizeof(mvp_matrix_uniform) % 4 == 0, "Not a multiple of 4");
        m_global.queue.WriteBuffer(state->object_uniform_buffer, 0, &mvp_matrix_uniform, sizeof(mvp_matrix_uniform));
    }
}

void render_system::setup_signals(ecs_registry &reg) {
    reg.on<comp_event::construct, mesh_data>().connect<&render_system::initialize_mesh_state>(*this);
    reg.on<comp_event::update, mesh_data>().connect<&render_system::create_mesh_state_from_data>(*this);
    reg.on<comp_event::destroy, mesh_data>().connect<&ecs_registry::remove_component<mesh_state>>();

    reg.on<comp_event::construct, rendered_mesh>().connect<&render_system::initialize_rendered_mesh_state>(*this);
    reg.on<comp_event::destroy, rendered_mesh>().connect<&ecs_registry::remove_component<rendered_mesh_state>>();
    make_soft_dependency<transform, rendered_mesh>(reg);

    make_soft_dependency<transform, camera>(reg);

    reg.on<comp_event::construct, texture_2d_data>().connect<&render_system::initialize_texture_2d_state>(*this);
    reg.on<comp_event::update, texture_2d_data>().connect<&render_system::create_texture_2d_state_from_data>(*this);
    reg.on<comp_event::destroy, texture_2d_data>().connect<&ecs_registry::remove_component<texture_2d_state>>();

    reg.on<comp_event::construct, material_data>().connect<&render_system::initialize_material_state>(*this);
    reg.on<comp_event::update, material_data>().connect<&render_system::create_material_state_from_data>(*this);
    reg.on<comp_event::destroy, material_data>().connect<&ecs_registry::remove_component<material_state>>();
}

void render_system::initialize_mesh_state(ecs_registry &reg, entity en) {
    reg.add_component<const mesh_state>(en);
    create_mesh_state_from_data(reg, en);
}

void render_system::create_mesh_state_from_data(ecs_registry &reg, entity en) const {
    auto [state, data] = reg.get_components<mesh_state, const mesh_data>(en);
    // Vertex buffer
    {
        const auto vertex_size_byte = data->vertices.size() * sizeof(std::decay_t<decltype(data->vertices)>::value_type);
        assert(vertex_size_byte > 0 && "Empty vertices list");
        wgpu::BufferDescriptor buffer_desc{
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
            .size = vertex_size_byte,
            .mappedAtCreation = false,
        };
        state->vertex_buffer = m_global.device.CreateBuffer(&buffer_desc);
        assert(state->vertex_buffer && "Failed to create buffer");
        assert(vertex_size_byte % 4 == 0 && "Size is a not multiple of 4");
        m_global.queue.WriteBuffer(state->vertex_buffer, 0, static_cast<const void *>(data->vertices.data()), vertex_size_byte);
    }
    // Index buffer
    if(data->maybe_indices.has_value()) {
        const auto index_size_byte = data->maybe_indices->size() * sizeof(std::decay_t<decltype(data->maybe_indices.value())>::value_type);
        wgpu::BufferDescriptor buffer_desc{
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
            .size = index_size_byte,
            .mappedAtCreation = false,
        };
        state->index_buffer = m_global.device.CreateBuffer(&buffer_desc);
        assert(state->index_buffer && "Failed to create buffer");
        assert(index_size_byte % 4 == 0 && "Size is a not multiple of 4");
        m_global.queue.WriteBuffer(state->index_buffer, 0, static_cast<const void *>(data->maybe_indices->data()), index_size_byte);
    }
}

void render_system::initialize_rendered_mesh_state(ecs_registry &reg, entity en) {
    assert(reg.has_components<mesh_data>(reg.get_components<const rendered_mesh>(en)->mesh_holder)
           && "rendered_mesh without mesh_data");
    assert(reg.has_components<material_data>(reg.get_components<const rendered_mesh>(en)->material_holder)
           && "rendered_mesh without material_data");

    auto state = reg.add_component<rendered_mesh_state>(en);
    state->object_uniform_buffer = create_object_uniform_buffer();
    state->object_bind_group = create_object_bind_group(state->object_uniform_buffer);
}

[[nodiscard]] wgpu::Buffer render_system::create_object_uniform_buffer() const {
    wgpu::BufferDescriptor buffer_desc{
        .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
        .size = sizeof(bind_group_layouts_data::object::mvp_matrix::type),
        .mappedAtCreation = false,
    };
    return m_global.device.CreateBuffer(&buffer_desc);
}

[[nodiscard]] wgpu::BindGroup render_system::create_object_bind_group(const wgpu::Buffer &uniform_buffer) const {
    std::array<wgpu::BindGroupEntry, bind_group_layouts_data::object::binding_count> entries = {
        wgpu::BindGroupEntry{
            .binding = bind_group_layouts_data::object::mvp_matrix::binding,
            .buffer = uniform_buffer,
            .offset = 0,
            .size = sizeof(bind_group_layouts_data::object::mvp_matrix::type),
        },
    };
    wgpu::BindGroupDescriptor desc{
        .layout = m_bind_group_layouts->object(),
        .entryCount = entries.size(),
        .entries = entries.data(),
    };
    return m_global.device.CreateBindGroup(&desc);
}

void render_system::initialize_texture_2d_state(ecs_registry &reg, entity en) const {
    reg.add_component<const texture_2d_state>(en);
    create_texture_2d_state_from_data(reg, en);
}

void render_system::create_texture_2d_state_from_data(ecs_registry &reg, entity en) const {
    auto [data, state] = reg.get_components<const texture_2d_data, texture_2d_state>(en);
    const wgpu::Extent3D texture_size{.width = data->size().x, .height = data->size().y, .depthOrArrayLayers = 1};
    // Create
    {
        assert(data->size().x > 0 && data->size().y > 0 && "Invalid texture size");
        const wgpu::TextureDescriptor desc{
            .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
            .dimension = wgpu::TextureDimension::e2D,
            .size = texture_size,
            .format = texture_2d_data::from_enum(data->texture_format()),
            .mipLevelCount = 1,
            .sampleCount = 1,
            .viewFormatCount = 0,
            .viewFormats = nullptr,
        };
        state->texture = m_global.device.CreateTexture(&desc);
    }
    // Write data
    {
        assert(data->data_ptr() && "No texture data");
        const wgpu::TexelCopyTextureInfo dest{
            .texture = state->texture,
            .mipLevel = 0,
            .origin = {.x = 0, .y = 0, .z = 0},
            .aspect = wgpu::TextureAspect::All,
        };
        const wgpu::TexelCopyBufferLayout layout{
            .offset = 0,
            .bytesPerRow = data->channel_count() * data->size().x,
            .rowsPerImage = data->size().y,
        };
        const std::size_t data_size_byte = static_cast<std::size_t>(data->channel_count() * data->size().x) * data->size().y;
        m_global.queue.WriteTexture(&dest, data->data_ptr(), data_size_byte, &layout, &texture_size);
    }
    // Texture view
    {
        const wgpu::TextureViewDescriptor desc{
            .format = texture_2d_data::from_enum(data->texture_format()),
            .dimension = wgpu::TextureViewDimension::e2D,
            .baseMipLevel = 0,
            .mipLevelCount = 1,
            .baseArrayLayer = 0,
            .arrayLayerCount = 1,
            .aspect = wgpu::TextureAspect::All,
            .usage = wgpu::TextureUsage::TextureBinding,
        };
        state->view = state->texture.CreateView(&desc);
    }
}

void render_system::initialize_material_state(ecs_registry &reg, entity en) const {
    reg.add_component<const material_state>(en);
    create_material_state_from_data(reg, en);
}

void render_system::create_material_state_from_data(ecs_registry &reg, entity en) const {
    auto [state, data] = reg.get_components<material_state, const material_data>(en);

    wgpu::TextureView texture_view;
    {
        if(data->texture_holder.is_null()) {
            texture_view = reg.get_components<const texture_2d_state>(default_texture_entity(reg))->view;
        } else {
            texture_view = reg.get_components<const texture_2d_state>(data->texture_holder)->view;
        }
    }
    const auto sampler_comp = reg.get_components<const sampler>(default_sampler_entity(reg));
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
        .layout = m_bind_group_layouts->material(),
        .entryCount = entries.size(),
        .entries = entries.data(),
    };
    state->material_bind_group = m_global.device.CreateBindGroup(&bind_group_desc);
}

struct default_texture_tag {};
entity render_system::default_texture_entity(ecs_registry &reg) {
    auto tagged_entities = reg.each<const default_texture_tag>();
    const auto is_initialized = tagged_entities.begin() != tagged_entities.end();
    if(!is_initialized) {
        auto result = reg.create_entity();
        constexpr auto max_value = std::numeric_limits<std::uint8_t>::max();
        reg.add_component<const texture_2d_data>(
            result,
            std::vector<std::uint8_t>(4, max_value),
            texture_2d_data::format::rgba8u_norm,
            vec2u{1, 1},
            4);
        reg.add_component<const default_texture_tag>(result);
        return result;
    }
    return std::get<0>(*tagged_entities.begin());
}

struct default_sampler_tag {};
entity render_system::default_sampler_entity(ecs_registry &reg) const {
    auto tagged_entities = reg.each<const default_sampler_tag>();
    const auto is_initialized = tagged_entities.begin() != tagged_entities.end();
    if(!is_initialized) {
        auto result = reg.create_entity();
        const wgpu::SamplerDescriptor desc{
            .addressModeU = wgpu::AddressMode::ClampToEdge,
            .addressModeV = wgpu::AddressMode::ClampToEdge,
            .addressModeW = wgpu::AddressMode::ClampToEdge,
            .magFilter = wgpu::FilterMode::Linear,
            .minFilter = wgpu::FilterMode::Linear,
            .mipmapFilter = wgpu::MipmapFilterMode::Linear,
            .lodMinClamp = 0.F,
            .lodMaxClamp = 1.F,
            .maxAnisotropy = 1,
        };
        reg.add_component<const sampler>(result, m_global.device.CreateSampler(&desc));
        return result;
    }
    return std::get<0>(*tagged_entities.begin());
}
} // namespace st
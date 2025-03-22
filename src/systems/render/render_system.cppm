module;

#include <algorithm>
#include <filesystem>
#include <tuple>
#include <type_traits>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:system;

import stay3.node;
import stay3.ecs;
import stay3.core;
import stay3.graphics;
import stay3.system.runtime_info;
import stay3.system.transform;
import :components;
import :config;
import :init_result;
import :bind_group_layouts;
import :pipeline;
import :render_pass;

export namespace st {
class render_system {
public:
    render_system(const vec2u &surface_size, std::filesystem::path shader_path, const render_config &config = {})
        : m_config{config}, m_surface_size{surface_size}, m_shader_path{std::move(shader_path)} {}
    void start(tree_context &ctx) {
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

    void render(tree_context &ctx) {
        auto &reg = ctx.ecs();
        const mat4f camera_view_projection = [](ecs_registry &reg) {
            auto cameras = reg.each<const camera, const main_camera, const global_transform>();
            assert(cameras.begin() != cameras.end() && "No main camera found");
            auto [en, cam, unused, tf] = *cameras.begin();
            return perspective(cam->fov, cam->ratio, cam->near, cam->far) * tf->get().inv_matrix();
        }(reg);
        update_all_object_uniforms(reg, camera_view_projection);
        // Draw commands
        const auto &&[unused, encoder, render_pass_encoder] = create_render_pass(m_global.device, m_global.surface, m_depth_texture.view);
        render_pass_encoder.SetPipeline(m_pipeline);
        for(auto &&[unused, data, state]: ctx.ecs().each<const rendered_mesh, const rendered_mesh_state>()) {
            auto [geometry_data, geometry_state] = ctx.ecs().get_components<const mesh_data, const mesh_state>(data->mesh_holder);
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

    void cleanup(tree_context &) const {
        m_global.surface.Unconfigure();
    }

private:
    void update_all_object_uniforms(ecs_registry &reg, const mat4f &camera_view_projection) {
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
    void setup_signals(ecs_registry &reg) {
        reg.on<comp_event::construct, mesh_data>().connect<&render_system::initialize_mesh_state>(*this);
        reg.on<comp_event::update, mesh_data>().connect<&render_system::create_mesh_state_from_data>(*this);
        reg.on<comp_event::destroy, mesh_data>().connect<&ecs_registry::remove_component<mesh_state>>();

        reg.on<comp_event::construct, rendered_mesh>().connect<&render_system::initialize_rendered_mesh_state>(*this);
        reg.on<comp_event::destroy, rendered_mesh>().connect<&ecs_registry::remove_component<rendered_mesh_state>>();
        make_soft_dependency<transform, rendered_mesh>(reg);

        make_soft_dependency<transform, camera>(reg);
    }

    void initialize_mesh_state(ecs_registry &reg, entity en) {
        reg.add_component<const mesh_state>(en);
        create_mesh_state_from_data(reg, en);
    }

    void initialize_rendered_mesh_state(ecs_registry &reg, entity en) {
        assert(reg.has_components<mesh_data>(reg.get_components<const rendered_mesh>(en)->mesh_holder)
               && "rendered_mesh without mesh_data");

        auto state = reg.add_component<rendered_mesh_state>(en);
        state->object_uniform_buffer = create_object_uniform_buffer();
        state->object_bind_group = create_object_bind_group(state->object_uniform_buffer);
    }

    void create_mesh_state_from_data(ecs_registry &reg, entity en) const {
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

    [[nodiscard]] wgpu::Buffer create_object_uniform_buffer() const {
        wgpu::BufferDescriptor buffer_desc{
            .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Uniform,
            .size = sizeof(bind_group_layouts_data::object::mvp_matrix::type),
            .mappedAtCreation = false,
        };
        return m_global.device.CreateBuffer(&buffer_desc);
    }

    [[nodiscard]] wgpu::BindGroup create_object_bind_group(const wgpu::Buffer &uniform_buffer) const {
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

    init_result m_global;
    texture_view m_depth_texture;
    wgpu::RenderPipeline m_pipeline;

    std::optional<bind_group_layouts> m_bind_group_layouts;

    render_config m_config;
    vec2u m_surface_size;
    std::filesystem::path m_shader_path;
};
} // namespace st
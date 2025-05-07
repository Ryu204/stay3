module;

#include <algorithm>
#include <filesystem>
#include <type_traits>
#include <variant>
#include <webgpu/webgpu_cpp.h>

module stay3.system.render;

import stay3.node;
import stay3.ecs;
import stay3.graphics;
import stay3.core;
import stay3.system.runtime_info;
import stay3.system.transform;

import :init_result;
import :pipeline;
import :render_pass;
import :material;
import :components;
import :mesh_subsystem;

namespace st {

render_system::render_system(const vec2u &surface_size, std::filesystem::path shader_path, const render_config &config)
    : m_config{config}, m_surface_size{surface_size}, m_shader_path{std::move(shader_path)} {}

void render_system::start(tree_context &ctx) {
    auto &window = ctx.vars().get<runtime_info>().window();
    m_global = create_and_config(window, m_config, m_surface_size);
    const texture_formats formats{
        .surface = m_global.surface_format,
        .depth = wgpu::TextureFormat::Depth24Plus,
    };
    m_depth_texture = create_depth_texture_view(m_global.device, m_surface_size, formats.depth);
    m_bind_group_layouts = bind_group_layouts{m_global.device};
    m_pipeline = create_pipeline(m_global.device, formats, m_shader_path, *m_bind_group_layouts);
    setup_signals(ctx);

    m_texture_subsystem.start(ctx, m_global);
    m_material_subsystem.start(ctx, m_global, m_config, m_bind_group_layouts->material());
    m_mesh_subsystem.start(ctx, m_global);
}

void render_system::render(tree_context &ctx) {
    m_texture_subsystem.process_commands(ctx);
    m_mesh_subsystem.process_pending_meshes(ctx);
    m_material_subsystem.process_pending_materials(ctx);
    auto &reg = ctx.ecs();
    vec4f clear_color;
    // Find main camera
    {
        auto cameras = reg.each<camera, main_camera, global_transform>();
        assert(cameras.begin() != cameras.end() && "No main camera found");
        auto [unused_en, cam, unused_tag, tf] = *cameras.begin();
        assert(cam->ratio.has_value() && "Camera aspect was not set by system");
        clear_color = cam->clear_color;
        const mat4f camera_projection = std::visit(
            visit_helper{
                [&cam](const camera::perspective_data &pers) {
                    return perspective(pers.fov, cam->ratio.value(), cam->near, cam->far);
                },
                [&cam](const camera::orthographic_data &ortho) {
                    return orthographic(ortho.width, cam->ratio.value(), cam->near, cam->far);
                }},
            cam->data);
        const auto camera_view_projection = camera_projection * tf->get().inv_matrix();
        update_all_object_uniforms(reg, camera_view_projection);
    }
    // Draw commands
    const auto &&[unused, encoder, render_pass_encoder] = create_render_pass(m_global.device, m_global.surface, m_depth_texture.view, clear_color);
    render_pass_encoder.SetPipeline(m_pipeline);

    component_ref<material> material;
    for(auto &&[unused, data, state]: reg.each<rendered_mesh, rendered_mesh_state>()) {
        if(data->mat != material) {
            material = data->mat;
            const auto material_bind_group = reg.get<material_state>(material.entity())->material_bind_group;
            render_pass_encoder.SetBindGroup(bind_group_layouts_data::material::group, material_bind_group);
        }
        auto [geometry_data, geometry_state] = reg.get<mesh_data, mesh_state>(data->mesh.entity());
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
    assert(std::ranges::all_of(reg.each<rendered_mesh_state>(), [&reg](const auto &tuple) {
               return reg.contains<global_transform>(std::get<0>(tuple));
           })
           && "rendered_mesh_state without global_transform");
    for(auto &&[unused, global_tf, state]: reg.each<global_transform, rendered_mesh_state>()) {
        const auto &mat = global_tf->get().matrix();
        static_assert(std::is_same_v<std::decay_t<decltype(mat)>, mat4f>);
        const mat4f mvp_matrix_uniform = camera_view_projection * global_tf->get().matrix();
        static_assert(sizeof(mvp_matrix_uniform) % 4 == 0, "Not a multiple of 4");
        m_global.queue.WriteBuffer(state->object_uniform_buffer, 0, &mvp_matrix_uniform, sizeof(mvp_matrix_uniform));
    }
}

void render_system::setup_signals(tree_context &ctx) {
    auto &reg = ctx.ecs();

    reg.on<comp_event::construct, rendered_mesh>().connect<&render_system::initialize_rendered_mesh_state>(*this);
    reg.on<comp_event::update, rendered_mesh>().connect<&render_system::validate_rendered_mesh>();
    reg.on<comp_event::destroy, rendered_mesh>().connect<&ecs_registry::destroy_if_exist<rendered_mesh_state>>();
    make_soft_dependency<transform, rendered_mesh>(reg);

    make_soft_dependency<transform, camera>(reg);
    reg.on<comp_event::construct, camera>().connect<&render_system::fix_camera_aspect>(ctx);

    register_mesh_builders<
        mesh_plane_builder,
        mesh_sprite_builder,
        mesh_cube_builder>(reg);
}

void render_system::fix_camera_aspect(tree_context &ctx, ecs_registry &reg, entity en) {
    auto &window = ctx.vars().get<runtime_info>().window();
    const auto aspect = static_cast<float>(window.size().x) / static_cast<float>(window.size().y);
    auto cam = reg.get<mut<camera>>(en);
    cam->ratio = aspect;
}

void render_system::validate_rendered_mesh(ecs_registry &reg, entity en) {
    assert(!reg.get<rendered_mesh>(en)->mesh.is_null()
           && "rendered_mesh without mesh_data");
    assert(mesh_subsystem::has_mesh(reg, reg.get<rendered_mesh>(en)->mesh.entity())
           && "rendered_mesh points to entity without a mesh");
    assert(!reg.get<rendered_mesh>(en)->mat.is_null()
           && "rendered_mesh without material");
}

void render_system::initialize_rendered_mesh_state(ecs_registry &reg, entity en) {
    validate_rendered_mesh(reg, en);
    auto state = reg.emplace<mut<rendered_mesh_state>>(en);
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
} // namespace st
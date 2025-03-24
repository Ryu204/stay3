module;

#include <filesystem>
#include <optional>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:system;

import stay3.core;
import stay3.node;
import stay3.ecs;
import :init_result;
import :config;
import :render_pass;
import :bind_group_layouts;

export namespace st {
class render_system {
public:
    render_system(const vec2u &surface_size, std::filesystem::path shader_path, const render_config &config = {});
    void start(tree_context &ctx);
    void render(tree_context &ctx);
    void cleanup(tree_context &) const;

private:
    void update_all_object_uniforms(ecs_registry &reg, const mat4f &camera_view_projection);
    void setup_signals(ecs_registry &reg);

    void initialize_mesh_state(ecs_registry &reg, entity en);
    void create_mesh_state_from_data(ecs_registry &reg, entity en) const;
    void initialize_rendered_mesh_state(ecs_registry &reg, entity en);
    [[nodiscard]] wgpu::Buffer create_object_uniform_buffer() const;
    [[nodiscard]] wgpu::BindGroup create_object_bind_group(const wgpu::Buffer &uniform_buffer) const;
    void initialize_texture_2d_state(ecs_registry &reg, entity en) const;
    void create_texture_2d_state_from_data(ecs_registry &reg, entity en) const;
    void initialize_material_state(ecs_registry &reg, entity en) const;
    void create_material_state_from_data(ecs_registry &reg, entity en) const;
    static entity default_texture_entity(ecs_registry &reg);
    entity default_sampler_entity(ecs_registry &reg) const;

    init_result m_global;
    texture_view m_depth_texture;
    wgpu::RenderPipeline m_pipeline;

    std::optional<bind_group_layouts> m_bind_group_layouts;

    render_config m_config;
    vec2u m_surface_size;
    std::filesystem::path m_shader_path;
};
} // namespace st
module;

#include <filesystem>
#include <optional>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render;

import stay3.core;
import stay3.node;
import stay3.ecs;
import stay3.system.render.priv;
export import stay3.system.render.config;

export namespace st {

class render_system {
public:
    render_system(const vec2u &surface_size, std::filesystem::path shader_path, const render_config &config = {});
    void start(tree_context &ctx);
    void render(tree_context &ctx);
    void cleanup(tree_context &) const;

private:
    void update_all_object_uniforms(ecs_registry &reg, const mat4f &camera_view_projection);
    void setup_signals(tree_context &ctx);

    static void fix_camera_aspect(tree_context &ctx, ecs_registry &reg, entity en);
    static void validate_rendered_mesh(ecs_registry &reg, entity en);
    void initialize_rendered_mesh_state(ecs_registry &reg, entity en);
    [[nodiscard]] wgpu::Buffer create_object_uniform_buffer() const;
    [[nodiscard]] wgpu::BindGroup create_object_bind_group(const wgpu::Buffer &uniform_buffer) const;

    init_result m_global;
    texture_view m_depth_texture;
    wgpu::RenderPipeline m_pipeline;

    std::optional<bind_group_layouts> m_bind_group_layouts;

    render_config m_config;
    vec2u m_surface_size;
    std::filesystem::path m_shader_path;
    texture_subsystem m_texture_subsystem;
    material_subsystem m_material_subsystem;
    mesh_subsystem m_mesh_subsystem;
};
} // namespace st
module;

#include <filesystem>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:pipeline;

import :bind_group_layouts;

export namespace st {

struct texture_formats {
    wgpu::TextureFormat surface;
    wgpu::TextureFormat depth;
};

wgpu::RenderPipeline create_pipeline(
    const wgpu::Device &device,
    const texture_formats &texture_formats,
    const std::filesystem::path &shader_path,
    const bind_group_layouts &layouts,
    bool culling);
} // namespace st
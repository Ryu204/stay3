module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:material;

export namespace st {
struct texture_2d_state {
    wgpu::Texture texture;
    wgpu::TextureView view;
};

struct sampler {
    wgpu::Sampler sampler;
};

struct material_state {
    wgpu::BindGroup material_bind_group;
};
}; // namespace st
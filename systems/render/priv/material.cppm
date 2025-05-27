module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render.priv:material;

import stay3.core;
import stay3.ecs;

export namespace st {
struct texture_2d_state {
    wgpu::Texture texture;
    wgpu::TextureView view;
};

struct sampler {
    wgpu::Sampler sampler;
};

struct material_uniform {
    vec4f color;
};

struct material_state {
    wgpu::BindGroup material_bind_group;
    entity sampler_entity;
    entity texture_entity;
    wgpu::Buffer properties_buffer;
};
}; // namespace st
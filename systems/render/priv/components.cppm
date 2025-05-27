module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render.priv:components;

import stay3.core;

export namespace st {

struct mesh_state {
    wgpu::Buffer vertex_buffer;
    wgpu::Buffer index_buffer;
};

struct rendered_mesh_state {
    wgpu::Buffer object_uniform_buffer;
    wgpu::BindGroup object_bind_group;
};

struct default_texture_tag {};
struct default_sampler_tag {};

} // namespace st
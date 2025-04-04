module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:components;

import stay3.core;

export namespace st {

struct mesh_data_changed {};
struct mesh_data_update_requested {};

struct mesh_state {
    wgpu::Buffer vertex_buffer;
    wgpu::Buffer index_buffer;
};

struct rendered_mesh_state {
    wgpu::Buffer object_uniform_buffer;
    wgpu::BindGroup object_bind_group;
};

} // namespace st
module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render.priv:init_result;

import stay3.core;
import stay3.graphics.core;
import stay3.system.render.config;

export namespace st {
struct init_result {
    wgpu::Device device;
    wgpu::Queue queue;
    wgpu::Surface surface;
    wgpu::TextureFormat surface_format;
};
init_result create_and_config(glfw_window &window, const render_config &config, const vec2u &surface_size);
} // namespace st
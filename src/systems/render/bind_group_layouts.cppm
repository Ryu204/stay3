module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:bind_group_layouts;

import stay3.core;
import :components;

namespace st {

template<typename entry>
wgpu::BindGroupLayoutEntry create_bind_group_layout_entry(wgpu::ShaderStage visibility) {
    return {
        .binding = entry::binding,
        .visibility = visibility,
        .buffer = wgpu::BufferBindingLayout{
            .type = wgpu::BufferBindingType::Uniform,
            .hasDynamicOffset = false,
            .minBindingSize = sizeof(typename entry::type),
        },
    };
}

struct bind_group_layouts_data {
    static constexpr auto group_count = 1;
    struct object {
        static constexpr auto group = 0;
        static constexpr auto binding_count = 1;
        struct mvp_matrix {
            static constexpr auto binding = 0;
            using type = mat4f;
        };
        static std::array<wgpu::BindGroupLayoutEntry, binding_count> create_entries() {
            return {
                create_bind_group_layout_entry<mvp_matrix>(wgpu::ShaderStage::Vertex),
            };
        }
    };
    static std::array<wgpu::BindGroupLayout, group_count> create_group_layouts(const wgpu::Device &device) {
        const auto object_entries = object::create_entries();
        wgpu::BindGroupLayoutDescriptor object_layout_desc{
            .entryCount = object::binding_count,
            .entries = object_entries.data(),
        };
        return {
            device.CreateBindGroupLayout(&object_layout_desc),
        };
    };
};

class bind_group_layouts {
public:
    bind_group_layouts(const wgpu::Device &device)
        : m_layouts{bind_group_layouts_data::create_group_layouts(device)} {}
    [[nodiscard]] const auto &object() const {
        return m_layouts[bind_group_layouts_data::object::group];
    }
    [[nodiscard]] const auto &layouts() const {
        return m_layouts;
    }

private:
    std::array<wgpu::BindGroupLayout, bind_group_layouts_data::group_count> m_layouts;
};
} // namespace st
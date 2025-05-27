module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render.priv:bind_group_layouts;

import stay3.core;
import :components;
import :material;

namespace st {

enum class bind_type : std::uint8_t {
    buffer,
    texture,
    sampler,
};

template<typename entry, bind_type type>
wgpu::BindGroupLayoutEntry create_bind_group_layout_entry(wgpu::ShaderStage visibility) {
    wgpu::BindGroupLayoutEntry result{
        .binding = entry::binding,
        .visibility = visibility,
    };
    if constexpr(type == bind_type::buffer) {
        result.buffer = wgpu::BufferBindingLayout{
            .type = wgpu::BufferBindingType::Uniform,
            .hasDynamicOffset = false,
            .minBindingSize = sizeof(typename entry::type),
        };
    } else if(type == bind_type::texture) {
        result.texture = {
            .sampleType = wgpu::TextureSampleType::Float,
            .viewDimension = wgpu::TextureViewDimension::e2D,
            .multisampled = false,
        };
    } else if(type == bind_type::sampler) {
        result.sampler = {
            .type = wgpu::SamplerBindingType::Filtering,
        };
    }
    return result;
}

export struct bind_group_layouts_data {
    static constexpr auto group_count = 2;

    struct material {
        static constexpr auto group = 0;
        static constexpr auto binding_count = 3;
        struct texture {
            static constexpr auto binding = 0;
        };
        struct sampler {
            static constexpr auto binding = 1;
        };
        struct properties {
            static constexpr auto binding = 2;
            using type = material_uniform;
        };
        static std::array<wgpu::BindGroupLayoutEntry, binding_count> create_entries() {
            return {
                create_bind_group_layout_entry<texture, bind_type::texture>(wgpu::ShaderStage::Fragment),
                create_bind_group_layout_entry<sampler, bind_type::sampler>(wgpu::ShaderStage::Fragment),
                create_bind_group_layout_entry<properties, bind_type::buffer>(wgpu::ShaderStage::Fragment),
            };
        }
    };
    struct object {
        static constexpr auto group = 1;
        static constexpr auto binding_count = 1;
        struct mvp_matrix {
            static constexpr auto binding = 0;
            using type = mat4f;
        };
        static std::array<wgpu::BindGroupLayoutEntry, binding_count> create_entries() {
            return {
                create_bind_group_layout_entry<mvp_matrix, bind_type::buffer>(wgpu::ShaderStage::Vertex),
            };
        }
    };
    static std::array<wgpu::BindGroupLayout, group_count> create_group_layouts(const wgpu::Device &device) {
        const auto material_entries = material::create_entries();
        const wgpu::BindGroupLayoutDescriptor material_layout_desc{
            .label = "material",
            .entryCount = material_entries.size(),
            .entries = material_entries.data(),
        };
        const auto object_entries = object::create_entries();
        const wgpu::BindGroupLayoutDescriptor object_layout_desc{
            .label = "object",
            .entryCount = object::binding_count,
            .entries = object_entries.data(),
        };
        return {
            device.CreateBindGroupLayout(&material_layout_desc),
            device.CreateBindGroupLayout(&object_layout_desc),
        };
    };
};

export class bind_group_layouts {
public:
    bind_group_layouts(const wgpu::Device &device)
        : m_layouts{bind_group_layouts_data::create_group_layouts(device)} {}
    [[nodiscard]] const auto &object() const {
        return m_layouts[bind_group_layouts_data::object::group];
    }
    [[nodiscard]] const auto &material() const {
        return m_layouts[bind_group_layouts_data::material::group];
    }
    [[nodiscard]] const auto &all_layouts() const {
        return m_layouts;
    }

private:
    std::array<wgpu::BindGroupLayout, bind_group_layouts_data::group_count> m_layouts;
};
} // namespace st
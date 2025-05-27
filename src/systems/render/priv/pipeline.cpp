module;

#include <atomic>
#include <filesystem>
#include <optional>
#include <webgpu/webgpu_cpp.h>

module stay3.system.render.priv;

import stay3.core;
import stay3.graphics.core;
import :bind_group_layouts;
import :pipeline;
import :wait;

namespace st {

struct shader_modules {
    wgpu::ShaderModule vertex;
    wgpu::ShaderModule fragment;
};

std::optional<shader_modules> create_shader_modules(const wgpu::Instance &instance, const wgpu::Device &device, const std::filesystem::path &shader_path) {
    const auto maybe_source = read_file_as_str_nothrow(shader_path);
    if(!maybe_source.has_value()) {
        log::error("Shader file not found");
        return std::nullopt;
    }
    const auto &source = maybe_source.value();

    wgpu::ShaderSourceWGSL source_desc;
    source_desc.code = source.c_str();
    source_desc.sType = wgpu::SType::ShaderSourceWGSL;
    wgpu::ShaderModuleDescriptor vertex_desc{.nextInChain = &source_desc};

    device.PushErrorScope(wgpu::ErrorFilter::Validation);
    bool is_creation_successful{false};
    std::atomic_bool is_query_done{false};
    const auto shader_module = device.CreateShaderModule(&vertex_desc);
    auto fut = device.PopErrorScope(
        wgpu::CallbackMode::AllowSpontaneous,
        [&is_creation_successful, &shader_path, &is_query_done](wgpu::PopErrorScopeStatus status, wgpu::ErrorType type, const wgpu::StringView &message) {
            if(status != wgpu::PopErrorScopeStatus::Success) {
                log::warn("Failed to check shader module creation status");
            } else if(type == wgpu::ErrorType::NoError) {
                is_creation_successful = true;
            } else {
                log::error("Shader module creation from ", shader_path, " failed:\n", message, " (code ", static_cast<std::uint32_t>(status), ")");
            }
            is_query_done.store(true);
        });
    if(!wgpu_wait(instance, fut, is_query_done)) {
        return std::nullopt;
    }
    if(!is_creation_successful) {
        return std::nullopt;
    }

    return shader_modules{
        .vertex = shader_module,
        .fragment = shader_module,
    };
}

wgpu::RenderPipeline create_pipeline(
    const wgpu::Instance &instance,
    const wgpu::Device &device,
    const texture_formats &texture_formats,
    const std::filesystem::path &shader_path,
    const bind_group_layouts &layouts,
    bool culling

) {
    wgpu::PipelineLayoutDescriptor layout_desc{
        .bindGroupLayoutCount = layouts.all_layouts().size(),
        .bindGroupLayouts = layouts.all_layouts().data(),
    };
    const auto layout = device.CreatePipelineLayout(&layout_desc);
    const auto shader_modules = [&instance](const auto &device, const auto &path) {
        auto maybe_modules = create_shader_modules(instance, device, path);
        if(!maybe_modules.has_value()) {
            throw graphics_error{"Failed to create shader"};
        }
        return maybe_modules.value();
    }(device, shader_path);
    std::array<wgpu::VertexAttribute, 4> vertex_attribs = {
        wgpu::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x4,
            .offset = offsetof(vertex_attributes, color),
            .shaderLocation = 0,
        },
        wgpu::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x3,
            .offset = offsetof(vertex_attributes, position),
            .shaderLocation = 1,
        },
        wgpu::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x3,
            .offset = offsetof(vertex_attributes, normal),
            .shaderLocation = 2,
        },
        wgpu::VertexAttribute{
            .format = wgpu::VertexFormat::Float32x2,
            .offset = offsetof(vertex_attributes, uv),
            .shaderLocation = 3,
        },
    };
    wgpu::VertexBufferLayout vertex_buffer_layout{
        .stepMode = wgpu::VertexStepMode::Vertex,
        .arrayStride = sizeof(vertex_attributes),
        .attributeCount = vertex_attribs.size(),
        .attributes = vertex_attribs.data(),
    };
    wgpu::DepthStencilState depth_stencil{
        .format = texture_formats.depth,
        .depthWriteEnabled = true,
        .depthCompare = wgpu::CompareFunction::Less,
        .stencilReadMask = 0,
        .stencilWriteMask = 0,
    };
    wgpu::BlendState blend_state{
        .color = {
            // Alpha blending
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::SrcAlpha,
            .dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha,
        },
        .alpha = {
            .operation = wgpu::BlendOperation::Add,
            .srcFactor = wgpu::BlendFactor::Zero,
            .dstFactor = wgpu::BlendFactor::One,
        },
    };
    wgpu::ColorTargetState color_target{
        .format = texture_formats.surface,
        .blend = &blend_state,
        .writeMask = wgpu::ColorWriteMask::All,
    };
    wgpu::FragmentState fragment{
        .module = shader_modules.fragment,
        .entryPoint = "fs_main",
        .constantCount = 0,
        .constants = nullptr,
        .targetCount = 1,
        .targets = &color_target,
    };
    wgpu::RenderPipelineDescriptor desc{
        .layout = layout,
        .vertex = {
            .module = shader_modules.vertex,
            .entryPoint = "vs_main",
            .constantCount = 0,
            .constants = nullptr,
            .bufferCount = 1,
            .buffers = &vertex_buffer_layout,
        },
        .primitive = {
            .topology = wgpu::PrimitiveTopology::TriangleList,
            .stripIndexFormat = wgpu::IndexFormat::Undefined,
            .frontFace = wgpu::FrontFace::CCW,
            .cullMode = culling ? wgpu::CullMode::Back : wgpu::CullMode::None,
        },
        .depthStencil = &depth_stencil,
        .multisample = {
            .count = 1,
            .mask = std::numeric_limits<std::uint32_t>::max(),
            .alphaToCoverageEnabled = false,
        },
        .fragment = &fragment,
    };

    return device.CreateRenderPipeline(&desc);
}
} // namespace st
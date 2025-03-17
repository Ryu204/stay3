module;

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <type_traits>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render;

import stay3.node;
import stay3.ecs;
import stay3.core;
import stay3.graphics;
import stay3.system.runtime_info;

namespace st {

export struct render_config {
    enum class power_preference : std::uint8_t {
        low,
        high,
        undefined
    };
    static wgpu::PowerPreference from_enum(power_preference pref) {
        switch(pref) {
        case power_preference::high:
            return wgpu::PowerPreference::HighPerformance;
        case power_preference::low:
            return wgpu::PowerPreference::LowPower;
        case power_preference::undefined:
            return wgpu::PowerPreference::Undefined;
        }
    }

    power_preference power_pref{power_preference::low};
};

std::optional<wgpu::Adapter> create_adapter(const wgpu::Instance &instance, const wgpu::Surface &surface, const render_config &config) {
    wgpu::RequestAdapterOptions options;
    options.powerPreference = render_config::from_enum(config.power_pref);
    options.compatibleSurface = surface;

    std::optional<wgpu::Adapter> maybe_adapter;

    const auto adapter_future = instance.RequestAdapter(&options, wgpu::CallbackMode::AllowSpontaneous, [&maybe_adapter](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
        if(status != wgpu::RequestAdapterStatus::Success) {
            log::error("Request adapter failed: ", message);
            return;
        }
        maybe_adapter = std::move(adapter);
    });

    if(instance.WaitAny(adapter_future, 0) != wgpu::WaitStatus::Success) {
        log::error("Failed to wait for adapter");
    }
    return maybe_adapter;
}

wgpu::Limits get_required_limits() {
    wgpu::Limits result;
    result.maxVertexAttributes = 4;
    result.maxVertexBuffers = 1;
    result.maxVertexBufferArrayStride = sizeof(vertex_attributes);
    result.maxBindGroups = 0;
    result.maxUniformBuffersPerShaderStage = 0;
    result.maxUniformBufferBindingSize = 0;
    result.maxStorageBuffersPerShaderStage = 0;
    result.maxInterStageShaderVariables = 4;

    return result;
}

std::optional<wgpu::Device> create_device(const wgpu::Adapter &adapter) {
    wgpu::DeviceDescriptor desc;
    desc.label = "My device";
    desc.requiredFeatureCount = 0;
    desc.requiredFeatures = nullptr;

    auto limits = get_required_limits();
    desc.requiredLimits = &limits;

    desc.defaultQueue.label = "My queue";

    desc.SetDeviceLostCallback(
        wgpu::CallbackMode::AllowSpontaneous,
        [](const wgpu::Device &,
           wgpu::DeviceLostReason reason,
           wgpu::StringView message) {
            if(reason == wgpu::DeviceLostReason::Destroyed) {
                log::info("Device destroyed");
            } else {
                log::warn("Device lost: ", message);
            }
        });

    desc.SetUncapturedErrorCallback(
        [](const wgpu::Device &,
           wgpu::ErrorType err,
           wgpu::StringView message) {
            log::error("Uncaptured error: ", static_cast<std::underlying_type_t<decltype(err)>>(err), '\n', message);
        });

    std::optional<wgpu::Device> maybe_device;
    const auto device_fut = adapter.RequestDevice(
        &desc,
        wgpu::CallbackMode::AllowSpontaneous,
        [&maybe_device](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
            if(status != wgpu::RequestDeviceStatus::Success) {
                log::error("Request device failed: ", message);
                return;
            }
            maybe_device = std::move(device);
        });

    if(adapter.GetInstance().WaitAny(device_fut, 0) != wgpu::WaitStatus::Success) {
        log::error("Failed to wait for device");
    }
    return maybe_device;
}

struct ShaderModules {
    wgpu::ShaderModule vertex;
    wgpu::ShaderModule fragment;
};

ShaderModules create_shader_modules(const wgpu::Device &device, const std::filesystem::path &shader_path) {
    const auto maybe_source = read_file_as_str_nothrow(shader_path);
    if(!maybe_source.has_value()) {
        throw error{"Shader file not found"};
    }
    const auto &source = maybe_source.value();

    wgpu::ShaderModuleDescriptor vertex_desc;
    wgpu::ShaderSourceWGSL vertex_code_desc;
    vertex_code_desc.code = source.c_str();
    vertex_code_desc.sType = wgpu::SType::ShaderSourceWGSL;
    vertex_desc.nextInChain = &vertex_code_desc;

    const auto vertex = device.CreateShaderModule(&vertex_desc);
    const auto fragment = vertex; // NOLINT

    return {
        .vertex = vertex,
        .fragment = fragment,
    };
}

wgpu::RenderPipeline create_pipeline(const wgpu::Device &device, wgpu::TextureFormat surface_format, const std::filesystem::path &shader_path) {
    wgpu::RenderPipelineDescriptor desc;

    const auto shader_modules = create_shader_modules(device, shader_path);

    auto &vertex = desc.vertex;
    wgpu::VertexBufferLayout vertex_buffer_layout;
    constexpr auto attribute_count = 4;
    std::array<wgpu::VertexAttribute, attribute_count> attributes = {
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
    vertex_buffer_layout.attributeCount = 4;
    vertex_buffer_layout.attributes = attributes.data();
    vertex_buffer_layout.arrayStride = sizeof(vertex_attributes);
    vertex_buffer_layout.stepMode = wgpu::VertexStepMode::Vertex;
    vertex.bufferCount = 1;
    vertex.buffers = &vertex_buffer_layout;
    vertex.module = shader_modules.vertex;
    vertex.entryPoint = "vs_main";
    vertex.constantCount = 0;
    vertex.constants = nullptr;

    auto &primitive = desc.primitive;
    primitive.cullMode = wgpu::CullMode::Back;
    primitive.frontFace = wgpu::FrontFace::CCW;
    primitive.topology = wgpu::PrimitiveTopology::TriangleList;
    primitive.stripIndexFormat = wgpu::IndexFormat::Undefined;

    wgpu::FragmentState fragment;
    fragment.entryPoint = "fs_main";
    fragment.constantCount = 0;
    fragment.constants = nullptr;
    fragment.module = shader_modules.fragment;
    wgpu::BlendState blend;
    blend.alpha.srcFactor = wgpu::BlendFactor::Zero;
    blend.alpha.dstFactor = wgpu::BlendFactor::One;
    blend.alpha.operation = wgpu::BlendOperation::Add;
    blend.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
    blend.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
    blend.color.operation = wgpu::BlendOperation::Add;
    wgpu::ColorTargetState color_target;
    color_target.format = surface_format;
    color_target.blend = &blend;
    color_target.writeMask = wgpu::ColorWriteMask::All;
    fragment.targetCount = 1;
    fragment.targets = &color_target;
    desc.fragment = &fragment;

    desc.depthStencil = nullptr;

    auto &multisample = desc.multisample;
    multisample.count = 1;
    multisample.mask = std::numeric_limits<std::uint32_t>::max(); // All bits on
    multisample.alphaToCoverageEnabled = false;

    desc.layout = nullptr;

    return device.CreateRenderPipeline(&desc);
} // namespace st

void config_surface(const wgpu::Surface &surface, const wgpu::Device &device, wgpu::TextureFormat texture_format, vec2u size) {
    wgpu::SurfaceConfiguration config;
    config.height = size.y;
    config.width = size.x;
    config.format = texture_format;
    config.usage = wgpu::TextureUsage::RenderAttachment;
    config.device = device;
    config.presentMode = wgpu::PresentMode::Fifo;
    config.alphaMode = wgpu::CompositeAlphaMode::Opaque;
    surface.Configure(&config);
}

struct mesh_status {
    wgpu::Buffer vertex_buffer;
    wgpu::Buffer index_buffer;
};

export class render_system {
public:
    render_system(const vec2u &surface_size, std::filesystem::path shader_path, const render_config &config = {})
        : m_config{config}, m_surface_size{surface_size}, m_shader_path{std::move(shader_path)} {}
    void start(tree_context &ctx) {
        auto &window = ctx.ecs().get_context<runtime_info>().window();
        auto instance = wgpu::CreateInstance();

        m_surface = window.create_wgpu_surface(instance);
        assert(m_surface != nullptr && "Invalid surface");

        const auto adapter = create_adapter(instance, m_surface, m_config);
        if(!adapter.has_value()) {
            throw graphics_error{"Failed to create adapter"};
        }

        wgpu::AdapterInfo adapter_info;
        adapter->GetInfo(&adapter_info);
        log::info(
            "Adapter info:\n\tArchitecture : ", adapter_info.architecture,
            "\n\tDescription: ", adapter_info.description,
            "\n\tDevice: ", adapter_info.device,
            "\n\tVendor: ", adapter_info.vendor);

        const auto maybe_device = create_device(adapter.value());
        if(!maybe_device.has_value()) {
            throw graphics_error{"Failed to create device"};
        }
        m_device = maybe_device.value();
        m_queue = m_device.GetQueue();

        wgpu::SurfaceCapabilities surface_caps;
        m_surface.GetCapabilities(adapter.value(), &surface_caps);
        if(surface_caps.formatCount == 0) {
            throw graphics_error{"Surface does not have a texture format"};
        }
        const auto preferred_texture_format = *surface_caps.formats;
        config_surface(m_surface, m_device, preferred_texture_format, m_surface_size);

        m_pipeline = create_pipeline(m_device, preferred_texture_format, m_shader_path);
        setup_signals(ctx.ecs());
    }

    void render(tree_context &ctx) {
        // Create the texture view
        wgpu::SurfaceTexture texture;
        m_surface.GetCurrentTexture(&texture);
        if(texture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal
           && texture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
            throw graphics_error{"Failed to get surface texture"};
        }
        wgpu::TextureViewDescriptor view_desc;
        view_desc.nextInChain = nullptr;
        view_desc.label = "Surface texture view";
        view_desc.format = texture.texture.GetFormat();
        view_desc.dimension = wgpu::TextureViewDimension::e2D;
        view_desc.baseMipLevel = 0;
        view_desc.mipLevelCount = 1;
        view_desc.baseArrayLayer = 0;
        view_desc.arrayLayerCount = 1;
        view_desc.aspect = wgpu::TextureAspect::All;
        wgpu::TextureView texture_view = texture.texture.CreateView(&view_desc);

        // Create encoder and configure render pass
        const auto encoder = m_device.CreateCommandEncoder();
        wgpu::RenderPassDescriptor render_desc;

        wgpu::RenderPassColorAttachment color_attachment;
        color_attachment.view = texture_view;
        color_attachment.clearValue = wgpu::Color{.r = 0.9, .g = 0.2, .b = 0.1, .a = 0.0};
        color_attachment.loadOp = wgpu::LoadOp::Clear;
        color_attachment.storeOp = wgpu::StoreOp::Store;
        color_attachment.depthSlice = wgpu::kDepthSliceUndefined;

        render_desc.colorAttachmentCount = 1;
        render_desc.colorAttachments = &color_attachment;
        render_desc.depthStencilAttachment = nullptr;

        // Draw commands
        const auto render_pass_encoder = encoder.BeginRenderPass(&render_desc);
        render_pass_encoder.SetPipeline(m_pipeline);
        for(auto &&[unused, data, status]: ctx.ecs().each<const mesh_data, const mesh_status>()) {
            render_pass_encoder.SetVertexBuffer(0, status->vertex_buffer);
            if(status->index_buffer) {
                render_pass_encoder.SetIndexBuffer(status->index_buffer, wgpu::IndexFormat::Uint32);
                render_pass_encoder.DrawIndexed(data->maybe_indices->size(), 1, 0, 0, 0);
            } else {
                render_pass_encoder.Draw(data->vertices.size(), 1, 0, 0);
            }
        }
        render_pass_encoder.End();

        // Submit the command buffer
        const auto cmd_buffer = encoder.Finish();
        m_queue.Submit(1, &cmd_buffer);

#ifndef __EMSCRIPTEN__
        m_surface.Present();
#endif
    }

    void cleanup(tree_context &) {
        m_surface.Unconfigure();
    }

private:
    void setup_signals(ecs_registry &reg) {
        reg.on<comp_event::construct, mesh_data>().connect<&render_system::update_vertex_buffer>(*this);
        reg.on<comp_event::update, mesh_data>().connect<&render_system::update_vertex_buffer>(*this);
        reg.on<comp_event::destroy, mesh_data>().connect<&render_system::remove_mesh_status>();
    }

    void update_vertex_buffer(ecs_registry &reg, entity en) {
        if(!reg.has_components<mesh_status>(en)) {
            reg.add_component<const mesh_status>(en);
        }
        auto [status, data] = reg.get_components<mesh_status, const mesh_data>(en);
        // Vertex buffer
        {
            const auto vertex_size_byte = data->vertices.size() * sizeof(std::decay_t<decltype(data->vertices)>::value_type);
            assert(vertex_size_byte > 0 && "Empty vertices list");
            wgpu::BufferDescriptor buffer_desc;
            buffer_desc.mappedAtCreation = false;
            buffer_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex;
            buffer_desc.size = vertex_size_byte;
            status->vertex_buffer = m_device.CreateBuffer(&buffer_desc);
            assert(status->vertex_buffer && "Failed to create buffer");
            assert(vertex_size_byte % 4 == 0 && "Size is a not multiple of 4");
            m_queue.WriteBuffer(status->vertex_buffer, 0, static_cast<const void *>(data->vertices.data()), vertex_size_byte);
        }
        // Index buffer
        if(data->maybe_indices.has_value()) {
            const auto index_size_byte = data->maybe_indices->size() * sizeof(std::decay_t<decltype(data->maybe_indices.value())>::value_type);
            wgpu::BufferDescriptor buffer_desc;
            buffer_desc.mappedAtCreation = false;
            buffer_desc.usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index;
            buffer_desc.size = index_size_byte;
            status->index_buffer = m_device.CreateBuffer(&buffer_desc);
            assert(status->index_buffer && "Failed to create buffer");
            assert(index_size_byte % 4 == 0 && "Size is a not multiple of 4");
            m_queue.WriteBuffer(status->index_buffer, 0, static_cast<const void *>(data->maybe_indices->data()), index_size_byte);
        }
    }

    static void remove_mesh_status(ecs_registry &reg, entity en) {
        reg.remove_component<mesh_status>(en);
    }

    wgpu::Device m_device;
    wgpu::Queue m_queue;
    wgpu::Surface m_surface;
    wgpu::RenderPipeline m_pipeline;

    render_config m_config;
    vec2u m_surface_size;
    std::filesystem::path m_shader_path;
};
} // namespace st
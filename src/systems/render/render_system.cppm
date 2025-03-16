module;

#include <cstdint>
#include <optional>
#include <string>
#include <type_traits>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render;

import stay3.node;
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

std::optional<wgpu::Device> create_device(const wgpu::Adapter &adapter) {
    wgpu::DeviceDescriptor desc;
    desc.label = "My device";
    desc.requiredFeatureCount = 0;
    desc.requiredFeatures = nullptr;

    wgpu::Limits limits;
    limits.maxBindGroups = 0;
    limits.maxUniformBuffersPerShaderStage = 0;
    limits.maxUniformBufferBindingSize = 0;
    limits.maxStorageBuffersPerShaderStage = 0;
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

export class render_system {
public:
    render_system(const vec2u &surface_size, const render_config &config = {})
        : m_config{config}, m_surface_size{surface_size} {}
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
        assert(m_surface != nullptr && "Invalid surface");
    }

    void render(tree_context &ctx) {
        // Create the texture view
        wgpu::SurfaceTexture texture;
        m_surface.GetCurrentTexture(&texture);
        if(texture.status != wgpu::SurfaceGetCurrentTextureStatus::Success) {
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

        const auto render_pass_encoder = encoder.BeginRenderPass(&render_desc);

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
    wgpu::Device m_device;
    wgpu::Queue m_queue;
    wgpu::Surface m_surface;
    render_config m_config;
    vec2u m_surface_size;
};
} // namespace st
module;

#include <atomic>
#include <webgpu/webgpu_cpp.h>

module stay3.system.render.priv;

import stay3.core;
import stay3.graphics.core;
import stay3.system.render.config;
import :init_result;
import :wait;

namespace st {

std::optional<wgpu::Adapter> create_adapter(const wgpu::Instance &instance, const wgpu::Surface &surface, const render_config &config) {
    wgpu::RequestAdapterOptions options{
        .powerPreference = render_config::from_enum(config.power_pref),
        .compatibleSurface = surface,
    };

    std::optional<wgpu::Adapter> maybe_adapter;
    std::atomic_bool is_request_done{false};
    const auto adapter_future = instance.RequestAdapter(
        &options,
        wgpu::CallbackMode::AllowSpontaneous,
        [&maybe_adapter, &is_request_done](wgpu::RequestAdapterStatus status, wgpu::Adapter adapter, wgpu::StringView message) {
            if(status != wgpu::RequestAdapterStatus::Success) {
                log::error("Request adapter failed: ", message);
            } else {
                maybe_adapter = std::move(adapter);
            }
            is_request_done.store(true);
        });

    if(!wgpu_wait(instance, adapter_future, is_request_done)) {
        log::error("Failed to wait for adapter");
    }
    return maybe_adapter;
}

void log_adapter_info(const wgpu::Adapter &adapter) {
    wgpu::AdapterInfo adapter_info;
    adapter.GetInfo(&adapter_info);
    log::info(
        "Adapter info:\n\tArchitecture : ", adapter_info.architecture,
        "\n\tDescription: ", adapter_info.description,
        "\n\tDevice: ", adapter_info.device,
        "\n\tVendor: ", adapter_info.vendor);
}

std::optional<wgpu::Device> create_device(const wgpu::Instance &instance, const wgpu::Adapter &adapter) {
    wgpu::DeviceDescriptor desc{wgpu::DeviceDescriptor::Init{
        .label = "My device",
        .requiredFeatureCount = 0,
        .requiredFeatures = nullptr,
        .requiredLimits = nullptr,
        .defaultQueue = {.label = "My queue"},
    }};

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
            log::error("Graphics device uncaptured error:\n", message, " (code ", static_cast<std::uint32_t>(err), ')');
        });

    std::optional<wgpu::Device> maybe_device;
    std::atomic_bool is_request_done{false};
    const auto device_fut = adapter.RequestDevice(
        &desc,
        wgpu::CallbackMode::AllowSpontaneous,
        [&maybe_device, &is_request_done](wgpu::RequestDeviceStatus status, wgpu::Device device, wgpu::StringView message) {
            if(status != wgpu::RequestDeviceStatus::Success) {
                log::error("Request device failed:\n", message, " (code ", static_cast<std::uint32_t>(status), ')');
            } else {
                maybe_device = std::move(device);
            }
            is_request_done.store(true);
        });

    if(!wgpu_wait(instance, device_fut, is_request_done)) {
        log::error("Failed to wait for device");
    }
    return maybe_device;
}

wgpu::TextureFormat get_first_surface_format(const wgpu::Surface &surface, const wgpu::Adapter &adapter) {
    wgpu::SurfaceCapabilities surface_caps;
    surface.GetCapabilities(adapter, &surface_caps);
    if(surface_caps.formatCount == 0) {
        throw graphics_error{"Surface does not have a texture format"};
    }
    return *surface_caps.formats;
}

void config_surface(const wgpu::Surface &surface, const wgpu::Device &device, wgpu::TextureFormat texture_format, vec2u size) {
    wgpu::SurfaceConfiguration config{
        .device = device,
        .format = texture_format,
        .usage = wgpu::TextureUsage::RenderAttachment,
        .width = size.x,
        .height = size.y,
        .viewFormatCount = 1,
        .viewFormats = &texture_format,
        .alphaMode = wgpu::CompositeAlphaMode::Opaque,
        .presentMode = wgpu::PresentMode::Fifo,
    };
    surface.Configure(&config);
}

init_result create_and_config(glfw_window &window, const render_config &config, const vec2u &surface_size) {
    const auto instance = wgpu::CreateInstance();
    if(!instance) {
        throw graphics_error{"Failed to create instance"};
    }
    const auto surface = window.create_wgpu_surface(instance);
    if(!surface) {
        throw graphics_error{"Failed to create surface"};
    }
    const auto maybe_adapter = create_adapter(instance, surface, config);
    if(!maybe_adapter.has_value()) {
        throw graphics_error{"Failed to create adapter"};
    }
    const auto &adapter = maybe_adapter.value();
    log_adapter_info(adapter);
    const auto maybe_device = create_device(instance, adapter);
    if(!maybe_device.has_value()) {
        throw graphics_error{"Failed to create device"};
    }
    const auto &device = maybe_device.value();
    const auto queue = device.GetQueue();
    const auto preferred_texture_format = get_first_surface_format(surface, adapter);
    config_surface(surface, device, preferred_texture_format, surface_size);
    return {
        .instance = instance,
        .device = device,
        .queue = queue,
        .surface = surface,
        .surface_format = preferred_texture_format,
    };
}

} // namespace st
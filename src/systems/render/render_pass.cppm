module;

#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:render_pass;

import stay3.core;
import stay3.graphics.core;

namespace st {

export struct texture_view {
    wgpu::Texture texture;
    wgpu::TextureView view;
};

export texture_view create_depth_texture_view(const wgpu::Device &device, const vec2u &size, wgpu::TextureFormat format) {
    texture_view result;
    wgpu::TextureDescriptor texture_desc{
        .usage = wgpu::TextureUsage::RenderAttachment,
        .dimension = wgpu::TextureDimension::e2D,
        .size = {.width = size.x, .height = size.y, .depthOrArrayLayers = 1},
        .format = format,
        .mipLevelCount = 1,
        .sampleCount = 1,
        .viewFormatCount = 1,
        .viewFormats = &format,
    };
    result.texture = device.CreateTexture(&texture_desc);
    wgpu::TextureViewDescriptor view_desc{
        .format = format,
        .dimension = wgpu::TextureViewDimension::e2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = wgpu::TextureAspect::DepthOnly,
        .usage = wgpu::TextureUsage::RenderAttachment,
    };
    result.view = result.texture.CreateView(&view_desc);
    return result;
}

texture_view create_surface_texture_view(const wgpu::Surface &surface) {
    wgpu::SurfaceTexture texture;
    surface.GetCurrentTexture(&texture);
    if(texture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal
       && texture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal) {
        throw graphics_error{"Failed to get surface texture"};
    }
    wgpu::TextureViewDescriptor view_desc{
        .format = texture.texture.GetFormat(),
        .dimension = wgpu::TextureViewDimension::e2D,
        .baseMipLevel = 0,
        .mipLevelCount = 1,
        .baseArrayLayer = 0,
        .arrayLayerCount = 1,
        .aspect = wgpu::TextureAspect::All,
    };
    wgpu::TextureView texture_view = texture.texture.CreateView(&view_desc);
    return {
        .texture = texture.texture,
        .view = texture_view,
    };
}

struct create_render_pass_result {
    texture_view color_texture_view;
    wgpu::CommandEncoder command_encoder;
    wgpu::RenderPassEncoder encoder;
};

export create_render_pass_result create_render_pass(const wgpu::Device &device, const wgpu::Surface &surface, const wgpu::TextureView &depth_texture_view, const vec4f &clear_color) {
    const texture_view color_texture_view{create_surface_texture_view(surface)};
    const wgpu::RenderPassColorAttachment color_attachment{
        .view = color_texture_view.view,
        .depthSlice = wgpu::kDepthSliceUndefined,
        .loadOp = wgpu::LoadOp::Clear,
        .storeOp = wgpu::StoreOp::Store,
        .clearValue = {
            .r = clear_color.r,
            .g = clear_color.g,
            .b = clear_color.b,
            .a = clear_color.a,
        },
    };
    const wgpu::RenderPassDepthStencilAttachment depth_stencil{
        .view = depth_texture_view,
        .depthLoadOp = wgpu::LoadOp::Clear,
        .depthStoreOp = wgpu::StoreOp::Store,
        .depthClearValue = 1.F,
        .depthReadOnly = false,
        .stencilLoadOp = wgpu::LoadOp::Undefined,
        .stencilStoreOp = wgpu::StoreOp::Undefined,
        .stencilClearValue = 0,
        .stencilReadOnly = true,
    };
    const wgpu::RenderPassDescriptor desc{
        .colorAttachmentCount = 1,
        .colorAttachments = &color_attachment,
        .depthStencilAttachment = &depth_stencil,
    };
    const auto command_encoder = device.CreateCommandEncoder();
    const auto encoder = command_encoder.BeginRenderPass(&desc);
    return {
        .color_texture_view = color_texture_view,
        .command_encoder = command_encoder,
        .encoder = encoder,
    };
}
} // namespace st
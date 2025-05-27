module;

#include <cassert>
#include <span>
#include <variant>
#include <webgpu/webgpu_cpp.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

export module stay3.system.render.priv:texture_subsystem;

import stay3.node;
import stay3.ecs;
import stay3.core;
import :init_result;
import :material;
import :components;

namespace st {
export class texture_subsystem {
public:
    void start(tree_context &tree_ctx, init_result &graphics_context) {
        m_context = &graphics_context;
        setup_signals(tree_ctx);
        create_default_texture_entity(tree_ctx);
    }

    void process_commands(tree_context &ctx) {
        assert(m_context != nullptr && "Texture subsystem not started yet");
        auto &reg = ctx.ecs();
        auto &cmds = ctx.vars().get<texture_2d::commands>();
        auto &queue = this->m_context->queue;

        const auto cmd_write = [&queue, &reg, m_context = this->m_context](const texture_2d::command_write &cmd) {
            auto [data, state] = reg.get<texture_2d, texture_2d_state>(cmd.target.entity());
            assert(!cmd.data.empty() && "No data to write to texture");
            assert(cmd.origin.x < data->size().x && cmd.origin.y < data->size().y && "Origin exceeds texture dimension");
            assert(
                (!cmd.size.has_value()
                 || (cmd.origin.x + cmd.size->x <= data->size().x && cmd.origin.y + cmd.size->y <= data->size().y))
                && "Region to write is not completely inside texture dimension");
            const auto size = cmd.size.value_or(data->size() - cmd.origin);
            assert(cmd.data.size() == static_cast<std::size_t>(data->channel_count()) * size.x * size.y && "Buffer does not match write size");
            const wgpu::TexelCopyTextureInfo dest{
                .texture = state->texture,
                .mipLevel = 0,
                .origin = {.x = cmd.origin.x, .y = cmd.origin.y, .z = 0},
                .aspect = wgpu::TextureAspect::All,
            };
            const wgpu::TexelCopyBufferLayout layout{
                .offset = 0,
                .bytesPerRow = data->channel_count() * size.x,
                .rowsPerImage = size.y,
            };
            const wgpu::Extent3D texture_size{
                .width = size.x,
                .height = size.y,
                .depthOrArrayLayers = 1,
            };
            const std::size_t data_size_byte =
                static_cast<std::size_t>(data->channel_count())
                * size.x * size.y;
            assert(cmd.data.size() == data_size_byte && "Data's size does not match texture region size");
            m_context->queue.WriteTexture(&dest, cmd.data.data(), data_size_byte, &layout, &texture_size);
        };
        const auto cmd_load = [&reg, &cmd_write](const texture_2d::command_load &cmd) {
            if(!is_normal_file(cmd.filename, "Image")) {
                return;
            }
            vec2i size;
            int raw_channel_count{};
            auto format = cmd.preferred_format.value_or(texture_2d::format::rgba8unorm);
            auto filename_str = cmd.filename.string();
            stbi_uc *loaded_data = stbi_load(filename_str.c_str(), &size.x, &size.y, &raw_channel_count, static_cast<int>(texture_2d::format_to_channel_count(format)));
            if(loaded_data == nullptr) {
                log::warn("Failed to load image: ", cmd.filename);
                return;
            }
            auto data = reg.emplace<texture_2d>(cmd.target, format, size);
            std::size_t data_size_byte =
                static_cast<std::size_t>(data->channel_count())
                * data->size().x * data->size().y;
            static_assert(std::is_same_v<stbi_uc, std::uint8_t>, "STBI unsigned char is not 8 bit unsigned int");
            const std::span<std::uint8_t> loaded_data_span{loaded_data, data_size_byte};
            cmd_write({.target = cmd.target, .data = {loaded_data_span.begin(), loaded_data_span.end()}});
            stbi_image_free(loaded_data);
        };
        const auto cmd_copy = [&reg](const texture_2d::command_copy &cmd) {
            assert(false && "Unimplemented");
        };

        while(!cmds.empty()) {
            const auto &cmd = cmds.front();
            std::visit(
                visit_helper{
                    cmd_load,
                    cmd_write,
                    cmd_copy,
                },
                cmd);
            cmds.pop();
        }
    }

private:
    void setup_signals(tree_context &ctx) {
        auto &reg = ctx.ecs();
        make_hard_dependency<texture_2d::commands, texture_2d>(reg);
        reg.on<comp_event::construct, texture_2d>().connect<&texture_subsystem::initialize_texture_2d_state>(*this);
        reg.on<comp_event::destroy, texture_2d>().connect<&ecs_registry::destroy_if_exist<texture_2d_state>>();
        auto &global = ctx.vars();
        global.emplace<texture_2d::commands>();
    }
    static void create_default_texture_entity(tree_context &ctx) {
        auto &reg = ctx.ecs();
        auto result = ctx.root().entities().create();
        constexpr auto max_value = std::numeric_limits<std::uint8_t>::max();
        reg.emplace<texture_2d>(
            result,
            texture_2d::format::rgba8unorm,
            vec2u{1, 1});
        reg.emplace<default_texture_tag>(result);
        ctx.vars().get<texture_2d::commands>().emplace(texture_2d::command_write{
            .target = result,
            .data = {max_value, max_value, max_value, max_value},
        });
    }
    void initialize_texture_2d_state(ecs_registry &reg, entity en) const {
        auto state = reg.emplace<mut<texture_2d_state>>(en);
        auto data = reg.get<texture_2d>(en);
        const wgpu::Extent3D texture_size{
            .width = data->size().x,
            .height = data->size().y,
            .depthOrArrayLayers = 1,
        };
        m_context->device.PushErrorScope(wgpu::ErrorFilter::Validation);
        // Create
        {
            assert(data->size().x > 0 && data->size().y > 0 && "Invalid texture size");
            const wgpu::TextureDescriptor desc{
                .usage = wgpu::TextureUsage::CopyDst | wgpu::TextureUsage::TextureBinding,
                .dimension = wgpu::TextureDimension::e2D,
                .size = texture_size,
                .format = texture_2d::from_enum(data->texture_format()),
                .mipLevelCount = 1,
                .sampleCount = 1,
                .viewFormatCount = 0,
                .viewFormats = nullptr,
            };
            state->texture = m_context->device.CreateTexture(&desc);
        }
        m_context->device.PopErrorScope(
            wgpu::CallbackMode::AllowSpontaneous,
            [](wgpu::PopErrorScopeStatus status, wgpu::ErrorType type, const wgpu::StringView &message) {
                if(status != wgpu::PopErrorScopeStatus::Success) {
                    log::warn("Failed to check texture creation status");
                    return;
                }
                if(type == wgpu::ErrorType::NoError) {
                    return;
                }
                log::error("Failed to create texture: ", message, " (code ", static_cast<std::uint32_t>(status), ")");
            });
        m_context->device.PushErrorScope(wgpu::ErrorFilter::Validation);
        // Texture view
        {
            const wgpu::TextureViewDescriptor desc{
                .format = texture_2d::from_enum(data->texture_format()),
                .dimension = wgpu::TextureViewDimension::e2D,
                .baseMipLevel = 0,
                .mipLevelCount = 1,
                .baseArrayLayer = 0,
                .arrayLayerCount = 1,
                .aspect = wgpu::TextureAspect::All,
                .usage = wgpu::TextureUsage::TextureBinding,
            };
            state->view = state->texture.CreateView(&desc);
        }
        m_context->device.PopErrorScope(
            wgpu::CallbackMode::AllowSpontaneous,
            [](wgpu::PopErrorScopeStatus status, wgpu::ErrorType type, const wgpu::StringView &message) {
                if(status != wgpu::PopErrorScopeStatus::Success) {
                    log::warn("Failed to check texture view creation status");
                    return;
                }
                if(type == wgpu::ErrorType::NoError) {
                    return;
                }
                log::error("Failed to create texture view: ", message, " (code ", static_cast<std::uint32_t>(status), ")");
            });
    }
    init_result *m_context{};
};
} // namespace st
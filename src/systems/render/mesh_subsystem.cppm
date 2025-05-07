module;

#include <cassert>
#include <webgpu/webgpu_cpp.h>

export module stay3.system.render:mesh_subsystem;

import stay3.node;
import stay3.ecs;
import stay3.core;
import stay3.graphics.core;
import :init_result;
import :components;

namespace st {

struct mesh_builder_data_changed {};
struct mesh_data_update_requested {};
struct mesh_data_changed {};

template<typename builder>
void register_one_mesh_builder(ecs_registry &reg) {
    reg.on<comp_event::construct, builder>().template connect<&ecs_registry::emplace<mesh_builder_data_changed>>();
    reg.on<comp_event::update, builder>().template connect<&ecs_registry::emplace_if_not_exist<mesh_builder_data_changed>>();
    reg.on<comp_event::destroy, builder>().template connect<&ecs_registry::destroy_if_exist<mesh_builder_data_changed>>();
    reg.on<comp_event::construct, mesh_data_update_requested>().connect<+[](ecs_registry &reg, entity en) {
        // This callback is invoked for every builder type so we need to do this check
        if(reg.contains<builder>(en)) {
            if constexpr(std::is_same_v<builder, mesh_sprite_builder>) {
                reg.emplace_or_replace<mesh_data>(en, reg.get<builder>(en)->build(reg));
            } else {
                reg.emplace_or_replace<mesh_data>(en, reg.get<builder>(en)->build());
            }
        }
    }>();
}

template<typename... builders>
void register_mesh_builders(ecs_registry &reg) {
    (register_one_mesh_builder<builders>(reg), ...);
}

export class mesh_subsystem {
public:
    void start(tree_context &tree_ctx, init_result &graphics_context) {
        m_context = &graphics_context;
        setup_signals(tree_ctx);
    }

    void process_pending_meshes(tree_context &ctx) {
        auto &reg = ctx.ecs();

        for(auto en: reg.view<mesh_builder_data_changed>()) {
            reg.emplace<mesh_data_update_requested>(en);
        }
        reg.destroy_all<mesh_builder_data_changed>();
        reg.destroy_all<mesh_data_update_requested>();

        for(auto en: reg.view<mesh_data_changed>()) {
            update_mesh_state_from_data(reg, en);
        }
        reg.destroy_all<mesh_data_changed>();
    }

    [[nodiscard]] static bool has_mesh(ecs_registry &reg, entity en) {
        return reg.contains<mesh_data>(en) || reg.contains<mesh_builder_data_changed>(en);
    }

private:
    static void setup_signals(tree_context &ctx) {
        auto &reg = ctx.ecs();
        make_hard_dependency<mesh_state, mesh_data>(reg);
        reg.on<comp_event::construct, mesh_data>().connect<&ecs_registry::emplace<mesh_data_changed>>();
        reg.on<comp_event::update, mesh_data>().connect<&ecs_registry::emplace_if_not_exist<mesh_data_changed>>();
        reg.on<comp_event::destroy, mesh_data>().connect<&ecs_registry::destroy_if_exist<mesh_data_changed>>();

        register_mesh_builders<
            mesh_plane_builder,
            mesh_sprite_builder,
            mesh_cube_builder>(reg);
    }
    void update_mesh_state_from_data(ecs_registry &reg, entity en) const {
        auto [state, data] = reg.get<mut<mesh_state>, mesh_data>(en);
        // Vertex buffer
        {
            const auto vertex_size_byte = data->vertices.size() * sizeof(std::decay_t<decltype(data->vertices)>::value_type);
            assert(vertex_size_byte > 0 && "Empty vertices list");
            wgpu::BufferDescriptor buffer_desc{
                .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Vertex,
                .size = vertex_size_byte,
                .mappedAtCreation = false,
            };
            state->vertex_buffer = m_context->device.CreateBuffer(&buffer_desc);
            assert(state->vertex_buffer && "Failed to create buffer");
            assert(vertex_size_byte % 4 == 0 && "Size is a not multiple of 4");
            m_context->queue.WriteBuffer(state->vertex_buffer, 0, static_cast<const void *>(data->vertices.data()), vertex_size_byte);
        }
        // Index buffer
        if(data->maybe_indices.has_value()) {
            const auto index_size_byte = data->maybe_indices->size() * sizeof(std::decay_t<decltype(data->maybe_indices.value())>::value_type);
            wgpu::BufferDescriptor buffer_desc{
                .usage = wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::Index,
                .size = index_size_byte,
                .mappedAtCreation = false,
            };
            state->index_buffer = m_context->device.CreateBuffer(&buffer_desc);
            assert(state->index_buffer && "Failed to create buffer");
            assert(index_size_byte % 4 == 0 && "Size is a not multiple of 4");
            m_context->queue.WriteBuffer(state->index_buffer, 0, static_cast<const void *>(data->maybe_indices->data()), index_size_byte);
        }
    }

    init_result *m_context;
};

} // namespace st
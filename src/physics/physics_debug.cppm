module;

#include <cassert>
#include <functional>
#include <ranges>
#include <vector>
// clang-format off
#include <Jolt/Jolt.h>
// clang-format on
#include <Jolt/Renderer/DebugRenderer.h>

export module stay3.physics.debug;

import stay3.core; /* DEBUG */
import stay3.node;
import stay3.ecs;
import stay3.graphics;
import stay3.physics.convert;
import stay3.system.transform;

namespace st {

class mesh_entity_wrapper
    : public JPH::RefTargetVirtual,
      public JPH::RefTarget<mesh_entity_wrapper> {
public:
    mesh_entity_wrapper(ecs_registry &reg)
        : m_registry{reg}, m_entity{reg.create()} {}
    mesh_entity_wrapper(const mesh_entity_wrapper &) = delete;
    mesh_entity_wrapper(mesh_entity_wrapper &&) noexcept = delete;
    mesh_entity_wrapper &operator=(const mesh_entity_wrapper &) = delete;
    mesh_entity_wrapper &operator=(mesh_entity_wrapper &&) noexcept = delete;
    ~mesh_entity_wrapper() override {
        // m_entity might be already destroyed because the tree was destroyed
        if(m_registry.get().contains(m_entity)) {
            m_registry.get().destroy(m_entity);
        }
    }

    [[nodiscard]] entity get_entity() const {
        return m_entity;
    }

    void AddRef() override {
        JPH::RefTarget<mesh_entity_wrapper>::AddRef();
    }
    void Release() override {
        JPH::RefTarget<mesh_entity_wrapper>::Release();
    }

private:
    std::reference_wrapper<ecs_registry> m_registry;
    entity m_entity;
};

struct debug_draw {};

export class physics_debug_drawer: public JPH::DebugRenderer {
public:
    physics_debug_drawer(tree_context &ctx)
        : m_tree_context{ctx} {
        JPH::DebugRenderer::Initialize();
    };

    // NOLINTNEXTLINE(*-easily-swappable-parameters)
    void DrawLine(JPH::RVec3Arg in, JPH::RVec3Arg to, JPH::ColorArg color) override {
        assert(false && "Unimplemented");
    }
    void DrawTriangle(JPH::RVec3Arg v1, JPH::RVec3Arg v2, JPH::RVec3Arg v3, JPH::ColorArg color, ECastShadow cast_shadow = ECastShadow::Off) override {
        assert(false && "Unimplemented");
    }
    void DrawText3D(JPH::RVec3Arg position, const JPH::string_view &content, JPH::ColorArg color = JPH::Color::sWhite, float inHeight = 0.5f) override {
        assert(false && "Unimplemented");
    }
    Batch CreateTriangleBatch([[maybe_unused]] const Triangle *triangles, int triangle_count) override {
        assert(false && "Unimplemented");

        return {};
    }
    Batch CreateTriangleBatch(const Vertex *vertices, int vertex_count, const JPH::uint32 *indices, int index_count) override {
        if(vertices == nullptr || vertex_count <= 0 || indices == nullptr || index_count <= 0) {
            return m_empty_batch;
        }
        auto *mesh_wrapper = new class mesh_entity_wrapper {
            m_tree_context.get().ecs()
        };
        auto mesh_entity = mesh_wrapper->get_entity();
        auto &reg = m_tree_context.get().ecs();
        std::span<const Vertex> vertices_span{vertices, static_cast<std::size_t>(vertex_count)};
        std::span<const JPH::uint32> indices_span{indices, static_cast<std::size_t>(index_count)};
        std::vector<vertex_attributes> vertices_attribs;
        vertices_attribs.reserve(vertices_span.size());
        for(const auto &attribs: vertices_span) {
            vertices_attribs.emplace_back(
                convert(attribs.mColor),
                convert(attribs.mPosition),
                convert(attribs.mNormal),
                convert(attribs.mUV));
        }
        reg.emplace<mesh_data>(
            mesh_entity,
            std::move(vertices_attribs),
            std::vector<std::uint32_t>{indices_span.begin(), indices_span.end()});
        return mesh_wrapper;
    }
    void DrawGeometry(JPH::RMat44Arg model_matrix, const JPH::AABox &world_space_bound, float lod_scale_squared, JPH::ColorArg model_color, const GeometryRef &geometry, ECullMode cull_mode = ECullMode::CullBackFace, ECastShadow cast_shadow = ECastShadow::On, EDrawMode draw_mode = EDrawMode::Solid) override {
        auto &reg = m_tree_context.get().ecs();
        auto inactives = reg.view<debug_draw>(exclude<rendered_mesh>);
        entity render_entity;
        const auto batch = geometry->GetLOD(convert(m_camera_position), world_space_bound, lod_scale_squared).mTriangleBatch;
        const auto *mesh_wrapper = dynamic_cast<const mesh_entity_wrapper *>(batch.GetPtr());
        if(inactives.begin() != inactives.end()) {
            render_entity = *inactives.begin();
        } else {
            render_entity = new_entity();
            reg.emplace<debug_draw>(render_entity);
        }
        reg.emplace<rendered_mesh>(
            render_entity,
            rendered_mesh{
                .mesh = mesh_wrapper->get_entity(),
                .mat = create_material(convert(model_color)),
            });
        set_global_transform(m_tree_context, render_entity, transform{}.set_matrix(convert(model_matrix)));
    }
    void begin_draw() {
        auto &reg = m_tree_context.get().ecs();
        auto camera_en = reg.view<main_camera>().front();
        m_camera_position = reg.get<global_transform>(camera_en)->get().position();
        const auto visible_entities = std::ranges::to<std::vector<entity>>(reg.view<rendered_mesh, debug_draw>());
        for(auto en: visible_entities) {
            reg.destroy<rendered_mesh>(en);
        }
    }
    void reset_cache() {
        auto &reg = m_tree_context.get().ecs();
        auto visual_entities = std::ranges::to<std::vector<entity>>(reg.view<debug_draw>());
        for(auto en: visual_entities) {
            reg.destroy(en);
        }
        for(auto [unused, en]: m_material_entities) {
            reg.destroy(en);
        }
        m_material_entities.clear();
    }

private:
    node &node() {
        return m_tree_context.get().root();
    }
    entity new_entity() {
        return node().entities().create();
    }
    entity create_material(const vec4f &color) {
        const auto key = hash(color);
        auto it = m_material_entities.find(key);
        if(it != m_material_entities.end()) { return it->second; }
        auto &reg = m_tree_context.get().ecs();
        auto en = reg.create();
        reg.emplace<material>(en, material{.color = color, .transparency = true});
        m_material_entities.emplace(key, en);
        return en;
    }
    using color_hash = int;
    static color_hash hash(const vec4f &color) {
        static constexpr std::array bases = {
            1ull << 8,
            1ull << 16,
        };
        return static_cast<int>((color.r * bases[0]) + (color.g * bases[1]) + (color.b * bases[0] * bases[1]) + (color.a * bases[1] * bases[1]));
    }

    Batch m_empty_batch;
    std::reference_wrapper<tree_context> m_tree_context;

    vec3f m_camera_position;
    std::unordered_map<color_hash, entity> m_material_entities;
};
} // namespace st
module;

#include <cassert>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_set>

// clang-format off
#include <Jolt/Jolt.h>
// clang-format on
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyManager.h>
#include <Jolt/Physics/Body/BodyType.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/EActivation.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

export module stay3.physics:world;

import stay3.core;
import stay3.ecs;
import stay3.physics.debug;
import stay3.node;
import stay3.physics.convert;
import :contact_listener;
import :collider;
import :rigidbody;

namespace st {

namespace layer {

constexpr JPH::ObjectLayer moving{0};
constexpr JPH::ObjectLayer non_moving{1};
constexpr auto count = 2u;

struct object_object_filter: public JPH::ObjectLayerPairFilter {
    [[nodiscard]] bool ShouldCollide([[maybe_unused]] JPH::ObjectLayer inLayer1, [[maybe_unused]] JPH::ObjectLayer inLayer2) const override {
        return inLayer1 == moving || inLayer2 == moving;
    }
};

} // namespace layer

namespace broad_phase {

constexpr JPH::BroadPhaseLayer moving{0};
constexpr JPH::BroadPhaseLayer non_moving{1};
constexpr auto layer_count = 2u;

struct layer_impl: public JPH::BroadPhaseLayerInterface {
    [[nodiscard]] unsigned int GetNumBroadPhaseLayers() const override {
        return layer_count;
    };

    [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override {
        switch(inLayer) {
        case layer::moving:
            return moving;
        case layer::non_moving:
            return non_moving;
        default:
            assert(false && "Invalid value");
        }
        return non_moving;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    [[nodiscard]] const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override {
        switch(static_cast<std::uint8_t>(inLayer)) {
        case static_cast<std::uint8_t>(moving):
            return "moving";
        case static_cast<std::uint8_t>(non_moving):
            return "non_moving";
        default:
            assert(false && "Invalid value");
        }
        return "moving";
    }
#endif
};

struct object_bp_layer_filter: public JPH::ObjectVsBroadPhaseLayerFilter {
    [[nodiscard]] bool ShouldCollide([[maybe_unused]] JPH::ObjectLayer inLayer1, [[maybe_unused]] JPH::BroadPhaseLayer inLayer2) const override {
        return inLayer1 == layer::moving || inLayer2 == moving;
    }
};

} // namespace broad_phase

class jolt_context {
public:
    jolt_context() {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = &factory;
        JPH::RegisterTypes();
    }
    ~jolt_context() {
        JPH::UnregisterTypes();
        JPH::Factory::sInstance = nullptr;
    }
    jolt_context(const jolt_context &) = delete;
    jolt_context(jolt_context &&) noexcept = delete;
    jolt_context &operator=(const jolt_context &) = delete;
    jolt_context &operator=(jolt_context &&) noexcept = delete;

private:
    JPH::Factory factory;
};

class jolt_context_user {
public:
    jolt_context_user()
        : m_context{
              []() {
                  static std::mutex mutex;
                  static std::weak_ptr<jolt_context> weak_context;

                  const std::lock_guard lock{mutex};
                  auto context = weak_context.lock();
                  if(!context) {
                      context = std::make_shared<jolt_context>();
                      weak_context = context;
                  }
                  return context;
              }()} {}

private:
    std::shared_ptr<jolt_context> m_context;
};

export struct physics_config {
    static constexpr vec3f earth_gravity{0.F, -9.8F, 0.F};

    /** @brief Max amount of bodies in a world */
    unsigned int max_body_count{std::numeric_limits<std::uint16_t>::max()};
    float updates_per_second{120.F};
    unsigned int collision_steps{2u};
    vec3f gravity{earth_gravity};
    bool debug_draw{false};
};

export class physics_world: private jolt_context_user {
public:
    using body_id = JPH::BodyID;
    constexpr physics_world(tree_context &ctx, const physics_config &settings = {})
        : m_contact_listener{ctx.ecs()}, m_settings{settings} {
        constexpr auto num_body_mutexes = 0u;
        constexpr auto max_body_pairs = std::numeric_limits<std::uint16_t>::max();
        constexpr auto max_contact_constraints = 10240u;

        m_physics_system.Init(
            settings.max_body_count,
            num_body_mutexes,
            max_body_pairs,
            max_contact_constraints,
            m_broad_phase_layer,
            m_object_bp_layer_filter,
            m_object_object_filter);
        m_physics_system.SetGravity(convert(settings.gravity));
        m_physics_system.SetContactListener(&m_contact_listener);

        if(settings.debug_draw) {
            m_drawer = std::make_unique<draw>(ctx);
        }
    }

    void update(float delta) {
        m_pending_time += delta;
        const seconds time_per_update = 1.F / m_settings.updates_per_second;

        m_bodies_with_changed_state.clear();
        m_contact_listener.removed_contacts.clear();
        while(m_pending_time > time_per_update) {
            m_pending_time -= time_per_update;
            const auto active_bodies_count = m_physics_system.GetNumActiveBodies(JPH::EBodyType::RigidBody);
            JPH::BodyIDVector this_update_bodies_list;
            m_physics_system.GetActiveBodies(JPH::EBodyType::RigidBody, this_update_bodies_list);
            for(auto body: this_update_bodies_list) {
                m_bodies_with_changed_state.insert(body);
            }

            const auto error = m_physics_system.Update(time_per_update, static_cast<int>(m_settings.collision_steps), &m_temp_allocator, &m_job_system);
            switch(error) {
            case JPH::EPhysicsUpdateError::None:
                break;
            case JPH::EPhysicsUpdateError::BodyPairCacheFull:
                log::warn("Physics: Body pair cache is full");
                break;
            case JPH::EPhysicsUpdateError::ManifoldCacheFull:
                log::warn("Physics: Manifold cache is full");
                break;
            case JPH::EPhysicsUpdateError::ContactConstraintsFull:
                log::warn("Physics: Contact constraints buffer is full, incoming contacts will be ignored");
                break;
            default:
                assert(false && "Invalid value");
            }
        }
        for(const auto &contact: m_contact_listener.removed_contacts) {
            auto en1 = entity(contact.GetBody1ID());
            auto en2 = entity(contact.GetBody2ID());
            m_contact_listener.add_collision_exit(en1, en2);
        }
    }

    [[nodiscard]] const auto &bodies_with_changed_state() const {
        return m_bodies_with_changed_state;
    }

    body_id create(const collider &col, const vec3f &position, const quaternionf &orientation, const rigidbody &rg, entity en) {
        const auto shape_result = col.build_geometry();
        JPH::BodyCreationSettings settings{
            shape_result.Get(),
            convert(position),
            convert(orientation).Normalized(),
            from(rg.motion_type),
            rg.motion_type == rigidbody::type::fixed ? layer::non_moving : layer::moving,
        };
        settings.mIsSensor = rg.is_sensor;
        settings.mAllowSleeping = rg.allow_sleep;
        auto &interface = m_physics_system.GetBodyInterface();
        auto result = interface.CreateAndAddBody(settings, JPH::EActivation::Activate);
        assert(!result.IsInvalid() && "Invalid body. Maybe body limit reached?");
        static_assert(sizeof(JPH::uint64) >= sizeof(std::uint32_t), "Cannot hold entity in Jolt user data");
        interface.SetUserData(result, static_cast<JPH::uint64>(en.numeric()));
        return result;
    }

    void destroy(const body_id &id) {
        auto &interface = m_physics_system.GetBodyInterface();
        interface.RemoveBody(id);
        interface.DestroyBody(id);
    }

    [[nodiscard]] vec3f linear_velocity(const body_id &id) const {
        const auto &interface = m_physics_system.GetBodyInterface();
        return convert(interface.GetLinearVelocity(id));
    }

    void set_linear_velocity(const body_id &id, const vec3f &linear) {
        auto &interface = m_physics_system.GetBodyInterface();
        interface.SetLinearVelocity(id, convert(linear));
    }

    void set_angular_velocity(const body_id &id, const vec3f &axis, float value) {
        auto &interface = m_physics_system.GetBodyInterface();
        interface.SetAngularVelocity(id, value * convert(axis));
    }

    [[nodiscard]] vec3f angular_velocity(const body_id &id) const {
        const auto &interface = m_physics_system.GetBodyInterface();
        return convert(interface.GetAngularVelocity(id));
    }

    [[nodiscard]] transform transform(const body_id &id) const {
        const auto raw_tf = m_physics_system.GetBodyInterface().GetWorldTransform(id);
        return {convert(raw_tf.GetTranslation()), convert(raw_tf.GetQuaternion())};
    }

    void add_force(const body_id &id, const vec3f &force, const std::optional<vec3f> &point = std::nullopt) {
        auto &interface = m_physics_system.GetBodyInterface();
        if(point.has_value()) {
            interface.AddForce(id, convert(force), convert(point.value()));
        } else {
            interface.AddForce(id, convert(force));
        }
    }

    void set_transform(const body_id &id, const vec3f &position, const quaternionf &orientation) {
        m_physics_system
            .GetBodyInterface()
            .SetPositionAndRotation(id, convert(position), convert(orientation).Normalized(), JPH::EActivation::Activate);
    }

    [[nodiscard]] entity entity(const body_id &id) const {
        const auto number = static_cast<std::uint32_t>(m_physics_system.GetBodyInterface().GetUserData(id));
        return entity::from_numeric(number);
    }

    void render() {
        assert(m_drawer && "The world was not configured with debug draw");
        m_drawer->debug_drawer.begin_draw();
        m_physics_system.DrawBodies(m_drawer->settings, &m_drawer->debug_drawer);
    }

private:
    static JPH::EMotionType from(rigidbody::type type) {
        switch(type) {
        case rigidbody::type::fixed:
            return JPH::EMotionType::Static;
        case rigidbody::type::dynamic:
            return JPH::EMotionType::Dynamic;
        case rigidbody::type::kinematic:
            return JPH::EMotionType::Kinematic;
        default:
            assert(false && "Invalid value");
        }
    }
    JPH::PhysicsSystem m_physics_system;
    std::unordered_set<body_id> m_bodies_with_changed_state;
    // Mandatory objects to use JPH::PhysicsSystem
    layer::object_object_filter m_object_object_filter;
    broad_phase::layer_impl m_broad_phase_layer;
    broad_phase::object_bp_layer_filter m_object_bp_layer_filter;
    contact_listener m_contact_listener;
    static constexpr std::size_t temp_allocator_capacity_mb = 10;
    JPH::TempAllocatorImpl m_temp_allocator{temp_allocator_capacity_mb * 1024 * 1024};
    JPH::JobSystemThreadPool m_job_system{
        JPH::cMaxPhysicsJobs,
        JPH::cMaxPhysicsBarriers,
        static_cast<int>(std::thread::hardware_concurrency()),
    };
    // Others
    physics_config m_settings;
    seconds m_pending_time{0.F};
    struct draw {
        draw(tree_context &ctx, const JPH::BodyManager::DrawSettings &settings = {})
            : debug_drawer{ctx}, settings{settings} {}
        physics_debug_drawer debug_drawer;
        JPH::BodyManager::DrawSettings settings;
    };
    std::unique_ptr<draw> m_drawer;
};

} // namespace st
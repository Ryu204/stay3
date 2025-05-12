module;

#include <cassert>
#include <cstdint>
#include <limits>
#include <mutex>
#include <thread>

// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/Memory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
// clang-format on

export module stay3.physics:world;

import stay3.core;
import :contact_listener;

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
    /** @brief Max amount of bodies in a world */
    unsigned int max_body_count{std::numeric_limits<std::uint16_t>::max()};
    float updates_per_second{120.F};
    unsigned int collision_steps{2u};
};

export class physics_world: private jolt_context_user {
public:
    constexpr physics_world(const physics_config &settings = {})
        : m_settings{settings} {
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
        m_physics_system.SetContactListener(&m_contact_listener);
    }

    void update(float delta) {
        m_pending_time += delta;
        const seconds time_per_update = 1.F / m_settings.updates_per_second;

        while(m_pending_time > time_per_update) {
            m_pending_time -= time_per_update;
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
    }

private:
    JPH::PhysicsSystem m_physics_system;

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
    physics_config m_settings;
    seconds m_pending_time{0.F};
};

} // namespace st
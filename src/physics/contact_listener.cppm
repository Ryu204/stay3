module;
#include <functional>
#include <vector>
// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Body/Body.h>
// clang-format on

export module stay3.physics:contact_listener;

import stay3.ecs;
import stay3.core;
import stay3.physics.convert;

namespace st {

export struct collision {
    entity other;
    vec3f normal;
};

using collision_collection = std::vector<collision>;

export class collision_enter: public collision_collection {
    using collision_collection::collision_collection;
};
export class collision_exit_unprocessed: public std::vector<JPH::SubShapeIDPair> {
    using std::vector<JPH::SubShapeIDPair>::vector;
};
export class collision_exit: public std::vector<entity> {
    using std::vector<entity>::vector;
};
export class collision_stay: public collision_collection {
    using collision_collection::collision_collection;
};

// An example contact listener

/// A listener class that receives collision contact events. It can be registered through PhysicsSystem::SetContactListener.
/// Only a single contact listener can be registered. A common pattern is to create a contact listener that casts Body::GetUserData
/// to a game object and then forwards the call to a handler specific for that game object.
/// Typically this is done on both objects involved in a collision event.
///
/// Note that contact listener callbacks are called from multiple threads at the same time when all bodies are locked, this means you cannot
/// use PhysicsSystem::GetBodyInterface / PhysicsSystem::GetBodyLockInterface but must use PhysicsSystem::GetBodyInterfaceNoLock / PhysicsSystem::GetBodyLockInterfaceNoLock instead.
/// If you use a locking interface, the simulation will deadlock. You're only allowed to read from the bodies and you can't change physics state.
///
/// During OnContactRemoved you cannot access the bodies at all, see the comments at that function.

export class contact_listener: public JPH::ContactListener {
public:
    contact_listener(ecs_registry &reg)
        : m_registry{reg} {}

    /// Called after detecting a collision between a body pair, but before calling OnContactAdded and before adding the contact constraint.
    /// If the function rejects the contact, the contact will not be processed by the simulation.
    /// This is a rather expensive time to reject a contact point since a lot of the collision detection has happened already, make sure you
    /// filter out the majority of undesired body pairs through the ObjectLayerPairFilter that is registered on the PhysicsSystem.
    ///
    /// This function may not be called again the next update if a contact persists and no new contact pairs between sub shapes are found.
    ///
    /// Note that this callback is called when all bodies are locked, so don't use any locking functions! See detailed class description of ContactListener.
    ///
    /// Body 1 will have a motion type that is larger or equal than body 2's motion type (order from large to small: dynamic -> kinematic -> static). When motion types are equal, they are ordered by BodyID.
    ///
    /// The collision result (inCollisionResult) is reported relative to inBaseOffset.
    JPH::ValidateResult OnContactValidate(const JPH::Body &body_1, const JPH::Body &body_2, JPH::RVec3Arg base_offset, const JPH::CollideShapeResult &collision_result) override {
        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    /// Called whenever a new contact point is detected.
    ///
    /// Note that this callback is called when all bodies are locked, so don't use any locking functions! See detailed class description of ContactListener.
    ///
    /// Body 1 and 2 will be sorted such that body 1 ID < body 2 ID, so body 1 may not be dynamic.
    ///
    /// Note that only active bodies will report contacts, as soon as a body goes to sleep the contacts between that body and all other
    /// bodies will receive an OnContactRemoved callback, if this is the case then Body::IsActive() will return false during the callback.
    ///
    /// When contacts are added, the constraint solver has not run yet, so the collision impulse is unknown at that point.
    /// The velocities of inBody1 and inBody2 are the velocities before the contact has been resolved, so you can use this to
    /// estimate the collision impulse to e.g. determine the volume of the impact sound to play (see: EstimateCollisionResponse).
    void OnContactAdded(const JPH::Body &body_1, const JPH::Body &body_2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override {
        auto en1 = entity(body_1);
        auto en2 = entity(body_2);
        auto &reg = m_registry.get();

        if(reg.contains<collision_enter>(en1)) {
            reg.get<mut<collision_enter>>(en1)->emplace_back(en2, convert(manifold.mWorldSpaceNormal));
        }
        if(reg.contains<collision_enter>(en2)) {
            reg.get<mut<collision_enter>>(en2)->emplace_back(en1, convert(manifold.mWorldSpaceNormal));
        }
    }

    /// Called whenever a contact is detected that was also detected last update.
    ///
    /// Note that this callback is called when all bodies are locked, so don't use any locking functions! See detailed class description of ContactListener.
    ///
    /// Body 1 and 2 will be sorted such that body 1 ID < body 2 ID, so body 1 may not be dynamic.
    ///
    /// If the structure of the shape of a body changes between simulation steps (e.g. by adding/removing a child shape of a compound shape),
    /// it is possible that the same sub shape ID used to identify the removed child shape is now reused for a different child shape. The physics
    /// system cannot detect this, so may send a 'contact persisted' callback even though the contact is now on a different child shape. You can
    /// detect this by keeping the old shape (before adding/removing a part) around until the next PhysicsSystem::Update (when the OnContactPersisted
    /// callbacks are triggered) and resolving the sub shape ID against both the old and new shape to see if they still refer to the same child shape.
    void OnContactPersisted(const JPH::Body &body_1, const JPH::Body &body_2, const JPH::ContactManifold &manifold, JPH::ContactSettings &settings) override {
        auto en1 = entity(body_1);
        auto en2 = entity(body_2);
        auto &reg = m_registry.get();

        if(reg.contains<collision_stay>(en1)) {
            reg.get<mut<collision_stay>>(en1)->emplace_back(en2, convert(manifold.mWorldSpaceNormal));
        }
        if(reg.contains<collision_stay>(en2)) {
            reg.get<mut<collision_stay>>(en2)->emplace_back(en1, convert(manifold.mWorldSpaceNormal));
        }
    }

    /// Called whenever a contact was detected last update but is not detected anymore.
    ///
    /// You cannot access the bodies at the time of this callback because:
    /// - All bodies are locked at the time of this callback.
    /// - Some properties of the bodies are being modified from another thread at the same time.
    /// - The body may have been removed and destroyed (you'll receive an OnContactRemoved callback in the PhysicsSystem::Update after the body has been removed).
    ///
    /// Cache what you need in the OnContactAdded and OnContactPersisted callbacks and store it in a separate structure to use during this callback.
    /// Alternatively, you could just record that the contact was removed and process it after PhysicsSystem::Update.
    ///
    /// Body 1 and 2 will be sorted such that body 1 ID < body 2 ID, so body 1 may not be dynamic.
    ///
    /// The sub shape IDs were created in the previous simulation step, so if the structure of a shape changes (e.g. by adding/removing a child shape of a compound shape),
    /// the sub shape ID may not be valid / may not point to the same sub shape anymore.
    /// If you want to know if this is the last contact between the two bodies, use PhysicsSystem::WereBodiesInContact.
    void OnContactRemoved(const JPH::SubShapeIDPair &sub_shape_pair) override {
        removed_contacts.emplace_back(sub_shape_pair);
    }

    collision_exit_unprocessed removed_contacts;
    void add_collision_exit(entity e1, entity e2) {
        auto &reg = m_registry.get();
        if(reg.contains<collision_exit>(e1)) {
            reg.get<mut<collision_exit>>(e1)->emplace_back(e2);
        }
        if(reg.contains<collision_exit>(e2)) {
            reg.get<mut<collision_exit>>(e2)->emplace_back(e1);
        }
    }

private:
    static entity entity(const JPH::Body &body) {
        return entity::from_numeric(static_cast<std::uint32_t>(body.GetUserData()));
    }
    std::reference_wrapper<ecs_registry> m_registry;
};
} // namespace st
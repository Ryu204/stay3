module;

// clang-format off
#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/ContactListener.h>
// clang-format on

export module stay3.physics:contact_listener;

namespace st {

// An example contact listener
export class contact_listener: public JPH::ContactListener {
public:
    // See: ContactListener
    JPH::ValidateResult OnContactValidate(const JPH::Body &inBody1, const JPH::Body &inBody2, JPH::RVec3Arg inBaseOffset, const JPH::CollideShapeResult &inCollisionResult) override {
        // Allows you to ignore a contact before it is created (using layers to not make objects collide is cheaper!)
        return JPH::ValidateResult::AcceptAllContactsForThisBodyPair;
    }

    void OnContactAdded(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
    }

    void OnContactPersisted(const JPH::Body &inBody1, const JPH::Body &inBody2, const JPH::ContactManifold &inManifold, JPH::ContactSettings &ioSettings) override {
    }

    void OnContactRemoved(const JPH::SubShapeIDPair &inSubShapePair) override {
    }
};
} // namespace st
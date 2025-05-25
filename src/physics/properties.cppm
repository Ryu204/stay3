module;

#include <functional>

export module stay3.physics:properties;

import stay3.core;

import :world;

export namespace st {
class motion {
public:
    motion(physics_world &world, const physics_world::body_id &id)
        : world{world}, id{id} {}
    [[nodiscard]] vec3f linear_velocity() const {
        return world.get().linear_velocity(id);
    }

    /** @brief Magnitude of the returned vector is the angle value */
    [[nodiscard]] vec3f angular_velocity() const {
        return world.get().angular_velocity(id);
    }

    void set_linear_velocity(const vec3f &value) {
        world.get().set_linear_velocity(id, value);
    }

    void set_angular_velocity(const vec3f &axis, float value) {
        world.get().set_angular_velocity(id, axis, value);
    }

    void add_force(const vec3f &force, const std::optional<vec3f> &point = std::nullopt) {
        world.get().add_force(id, force, point);
    }

private:
    std::reference_wrapper<physics_world> world;
    physics_world::body_id id;
};
} // namespace st
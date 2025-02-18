module;

#include <cassert>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>

export module stay3.system:quaternion;

import :vector;
import :matrix;

export namespace st {

template<typename type>
struct quaternion: public glm::qua<type> {
    using base_glm = glm::qua<type>;
    using base_glm::base_glm;
    quaternion()
        : base_glm{static_cast<type>(1.F), static_cast<type>(0.F), static_cast<type>(0.F), static_cast<type>(0.F)} {}
    quaternion(const base_glm &quat)
        : base_glm{quat} {}
    quaternion(const vec3<type> &axis, type angle)
        : base_glm{glm::angleAxis(angle, axis)} {
        assert(axis.is_normalized() && "Quaternion axis must be normalized");
    }
    /**
     * @return this rotation followed by `other`
     */
    quaternion &rotate(const base_glm &other) {
        *this = other * *this;
        return *this;
    }
    quaternion &rotate(const vec3<type> &axis, type angle) {
        return rotate(quaternion{axis, angle});
    }
    [[nodiscard]] mat4<type> matrix() const {
        return glm::mat4_cast(*this);
    }
    [[nodiscard]] type angle() const {
        return glm::angle(*this);
    }
    [[nodiscard]] vec3<type> axis() const {
        return glm::axis(*this);
    }
    [[nodiscard]] type dot(const base_glm &other) const {
        return glm::dot(*this, other);
    }
};

using quaternionf = quaternion<float>;
} // namespace st
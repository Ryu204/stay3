module;

#include <cassert>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>

export module stay3.system:quaternion;

import :vector;
import :matrix;

export namespace st {

using radians = float;

template<typename type>
struct quaternion: public glm::qua<type> {
    using base_glm = glm::qua<type>;
    using base_glm::base_glm;
    quaternion()
        : base_glm{1.F, 0.F, 0.F, 0.F} {}
    quaternion(const base_glm &quat)
        : base_glm{quat} {}
    quaternion(const vec3<type> &axis, radians angle)
        : base_glm{glm::angleAxis(angle, axis)} {
        assert(axis.is_normalized() && "Quaternion axis must be normalized");
    }
    quaternion &rotate(const base_glm &other) {
        *this *= other;
        return *this;
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
};

using quaternionf = quaternion<float>;
} // namespace st
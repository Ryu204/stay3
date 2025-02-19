module;

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

export module stay3.system:vector;

import :math;

export namespace st {

template<std::size_t length, typename type>
struct base_vec: public glm::vec<length, type> {
    using base_glm = glm::vec<length, type>;

    using base_glm::base_glm;
    constexpr base_vec()
        : base_glm{0.F} {}
    constexpr base_vec(const base_glm &vec)
        : base_glm{vec} {}

    [[nodiscard]] constexpr type magnitude() const {
        return glm::length<length, type>(*this);
    }

    [[nodiscard]] constexpr bool is_normalized(type tolerance = static_cast<type>(EPS)) const {
        const auto len = glm::length2<length, type>(*this);
        return static_cast<type>(1.0) - tolerance <= len && len <= static_cast<type>(1.0) + tolerance;
    }

    [[nodiscard]] constexpr type magnitude_squared() const {
        return glm::length2<length, type>(*this);
    }

    [[nodiscard]] constexpr type dot(const base_glm &other) const {
        return glm::dot(*this, other);
    }

    [[nodiscard]] constexpr base_vec normalized() const {
        return glm::normalize(*this);
    }

    [[nodiscard]] constexpr base_vec cross(const base_glm &other) const {
        return glm::cross(*this, other);
    }
};

template<typename type>
using vec2 = base_vec<2, type>;
template<typename type>
using vec3 = base_vec<3, type>;
template<typename type>
using vec4 = base_vec<4, type>;

using vec2f = vec2<float>;
using vec2i = vec2<int>;
using vec3f = vec3<float>;
using vec3i = vec3<int>;
using vec4f = vec4<float>;
using vec4i = vec4<int>;

/* Left handed coordinate system
          Y+
              |
              |     SCREEN
              |
              |
              /------------------ X+
            /
          /
        /
      Z- <-- (PLAYER)
    */
constexpr vec3f vec_up{0.F, 1.F, 0.F};
constexpr vec3f vec_down{0.F, -1.F, 0.F};
constexpr vec3f vec_left{-1.F, 0.F, 0.F};
constexpr vec3f vec_right{1.F, 0.F, 0.F};
constexpr vec3f vec_back{0.F, 0.F, -1.F};
constexpr vec3f vec_forward{0.F, 0.F, 1.F};

} // namespace st

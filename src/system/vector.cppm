module;

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>

export module stay3.system:vector;

export namespace st {

template<std::size_t length, typename type>
struct base_vec: public glm::vec<length, type> {
    using base_glm = glm::vec<length, type>;

    using base_glm::base_glm;
    base_vec(const base_glm &vec)
        : base_glm{vec} {};

    type magnitude() const {
        return glm::length<length, type>(*this);
    }

    type magnitude_squared() const {
        return glm::length2<length, type>(*this);
    }

    type dot(const base_glm &other) const {
        return glm::dot(*this, other);
    }

    base_vec normalized() const {
        return glm::normalize(*this);
    }

    base_vec cross(const base_glm &other) const {
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

using glm::operator+;
using glm::operator-;
using glm::operator*;
using glm::operator/;

} // namespace st

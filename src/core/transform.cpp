module;

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

module stay3.core;

import :math_ops;

namespace st {
transform::transform(const vec3f &position, const quaternionf &orientation, const vec3f &scale)
    : m_position{position}, m_orientation{orientation}, m_scale{scale} {}

transform::transform(const transform &other)
    : m_position{other.m_position}, m_orientation{other.m_orientation}, m_scale{other.m_scale} {}

transform &transform::operator=(const transform &other) {
    m_position = other.m_position;
    m_orientation = other.m_orientation;
    m_scale = other.m_scale;
    if(other.m_transform_mat_ok) {
        m_transform_mat_ok = true;
        m_transform_mat = other.m_transform_mat;
    } else {
        m_transform_mat_ok = false;
    }
    if(other.m_inv_transform_mat_ok) {
        m_inv_transform_mat_ok = true;
        m_inv_transform_mat = other.m_inv_transform_mat;
    } else {
        m_inv_transform_mat_ok = false;
    }
    return *this;
}

transform &transform::rotate(const vec3f &axis, radians angle) {
    auto quat = quaternionf{axis.normalized(), angle};

    return rotate(quat);
}

transform &transform::rotate(const quaternionf &quat) {
    m_orientation.rotate(quat);

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;

    return *this;
}

transform &transform::translate(const vec3f &offset) {
    m_position += offset;

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;

    return *this;
}

transform &transform::scale(const vec3f &scale) {
    m_scale = m_scale * scale;

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;

    return *this;
}

transform &transform::scale(float scale) {
    return this->scale({scale, scale, scale});
}

transform &transform::set_orientation(const vec3f &axis, radians angle) {
    m_orientation = quaternionf{axis.normalized(), angle};

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;

    return *this;
}

transform &transform::set_orientation(const quaternionf &quat) {
    m_orientation = quat;

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;
    return *this;
}

transform &transform::set_position(const vec3f &pos) {
    m_position = pos;

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;
    return *this;
}

transform &transform::set_scale(const vec3f &scale) {
    m_scale = scale;

    m_transform_mat_ok = false;
    m_inv_transform_mat_ok = false;
    return *this;
}

transform &transform::set_scale(float scale) {
    return set_scale({scale, scale, scale});
}

transform &transform::set_matrix(const mat4f &mat) {
    vec3f skew;
    vec4f perspective;
    glm::decompose<float>(mat, m_scale, m_orientation, m_position, skew, perspective);
    m_transform_mat = mat;
    m_transform_mat_ok = true;
    m_inv_transform_mat_ok = false;
    return *this;
}

const mat4f &transform::matrix() const {
    if(!m_transform_mat_ok) {
        m_transform_mat = glm::translate(glm::mat4{1.F}, m_position);
        m_transform_mat *= m_orientation.matrix();
        m_transform_mat = glm::scale(m_transform_mat, m_scale);
        m_transform_mat_ok = true;
    }

    return m_transform_mat;
}

const mat4f &transform::inv_matrix() const {
    if(!m_inv_transform_mat_ok) {
        m_inv_transform_mat = matrix().inv();
        m_inv_transform_mat_ok = true;
    }

    return m_inv_transform_mat;
}

const quaternionf &transform::orientation() const {
    return m_orientation;
}

const vec3f &transform::position() const {
    return m_position;
}

const vec3f &transform::scale() const {
    return m_scale;
}

} // namespace st
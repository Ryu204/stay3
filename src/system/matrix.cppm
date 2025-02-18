module;

#include <glm/ext/matrix_transform.hpp>
#include <glm/matrix.hpp>

export module stay3.system:matrix;

import :vector;

export namespace st {
template<std::size_t col, std::size_t row, typename type>
struct mat: public glm::mat<col, row, type> {
    using base_glm = glm::mat<col, row, type>;
    using base_glm::base_glm;
    constexpr mat()
        : base_glm{static_cast<type>(1.0)} {}
    constexpr mat(const base_glm &mat)
        : base_glm{mat} {}

    [[nodiscard]] mat inv() const {
        return glm::inverse(*this);
    }

    mat &operator*=(const base_glm &other) {
        *this = *this * other;
        return *this;
    }

    /**
     * @example `mat.scale(...).rotate(...).translate(...);`
     */
    mat &translate(const vec3<type> &offset) {
        *this = glm::translate(mat{}, offset) * *this;
        return *this;
    }

    /**
     * @example `mat.translate(...).after_rotate(...).after_scale(...);`
     */
    mat &after_translate(const vec3<type> &offset) {
        *this = glm::translate(*this, offset);
        return *this;
    }

    /**
     * @param angle measured in radians
     */
    mat &rotate(const vec3<type> &axis, type angle) {
        *this = glm::rotate(mat{}, angle, axis) * *this;
        return *this;
    }

    mat &after_rotate(const vec3<type> &axis, type angle) {
        *this = glm::rotate(*this, angle, axis);
        return *this;
    }

    mat &scale(const vec3<type> &scale) {
        *this = glm::scale(mat{}, scale) * *this;
        return *this;
    }

    mat &after_scale(const vec3<type> &scale) {
        *this = glm::scale(*this, scale);
        return *this;
    }
};

template<typename type>
using mat4 = mat<4, 4, type>;

using mat4f = mat<4, 4, float>;
} // namespace st
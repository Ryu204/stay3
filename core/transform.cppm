module;

export module stay3.core:transform;

import :vector;
import :quaternion;
import :matrix;
import :math;

export namespace st {
/**
 * @brief Translate-rotation-scale matrix with separate components
 * @note This class is not thread-safe
 */
class transform {
public:
    transform(const vec3f &position = {}, const quaternionf &orientation = {}, const vec3f &scale = vec3f{1.F});
    transform(const transform &);
    ~transform() = default;
    transform &operator=(const transform &other);
    /**
     * @brief Rotates the transform around `axis` by `angle`
     * @param axis Rotation axis
     * @param angle Rotation amount, clockwise when looking at origin from `axis`
     */
    transform &rotate(const vec3f &axis, radians angle);
    transform &rotate(const quaternionf &quat);
    transform &translate(const vec3f &offset);
    transform &scale(const vec3f &scale);
    transform &scale(float scale);

    transform &set_orientation(const vec3f &axis, radians angle);
    transform &set_orientation(const quaternionf &quat);
    transform &set_position(const vec3f &pos);
    transform &set_scale(const vec3f &scale);
    transform &set_scale(float scale);
    transform &set_matrix(const mat4f &mat);

    const mat4f &matrix() const;
    const mat4f &inv_matrix() const;

    const quaternionf &orientation() const;
    const vec3f &position() const;
    const vec3f &scale() const;

private:
    vec3f m_position{0.F};
    quaternionf m_orientation;
    vec3f m_scale{1.F};

    mutable mat4f m_transform_mat;
    mutable mat4f m_inv_transform_mat;

    mutable bool m_transform_mat_ok{false};
    mutable bool m_inv_transform_mat_ok{false};
};
} // namespace st
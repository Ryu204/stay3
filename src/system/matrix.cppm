module;

#include <glm/matrix.hpp>

export module stay3.system:matrix;

export namespace st {
template<std::size_t col, std::size_t row, typename type>
struct mat: public glm::mat<col, row, type> {
    using base_glm = glm::mat<col, row, type>;
    using base_glm::base_glm;
    constexpr mat()
        : base_glm{1.F} {}
    constexpr mat(const base_glm &mat)
        : base_glm{mat} {}

    [[nodiscard]] mat inv() const {
        return glm::inverse(*this);
    }

    mat &operator*=(const base_glm &other) {
        *this = *this * other;
        return *this;
    }
};

template<typename type>
using mat4 = mat<4, 4, type>;

using mat4f = mat<4, 4, float>;
} // namespace st
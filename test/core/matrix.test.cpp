#include <catch2/catch_all.hpp>
#include <glm/ext.hpp>
#include <glm/glm.hpp>
import stay3;
import stay3.test_helper;

TEST_CASE("Matrix operations", "[mat]") {
    using namespace st;

    SECTION("Matrix initialization should return identity") {
        constexpr mat4f identity{};
        REQUIRE(identity == glm::mat4{1.F});
    }

    SECTION("Matrix construction") {
        constexpr mat4f mata{3.F};
        REQUIRE(mata == glm::mat4{3.F});
    }

    SECTION("Matrix inversion") {
        constexpr mat4f matrix = glm::mat4{2.F};
        const mat4f inv_matrix = matrix.inv();
        REQUIRE(glm::inverse(matrix) == inv_matrix);
    }

    SECTION("Matrix multiplication") {
        mat4f mata{5.F};
        constexpr mat4f matb{2.F};
        const mat4f result = mata * matb;
        mata *= matb;
        REQUIRE(mata == result);
    }

    SECTION("Projection") {
        SECTION("Perspective") {
            const auto fov = 40 * PI / 180;
            const auto near = 0.01F;
            const auto far = 10.F;
            const auto aspect = 0.5F;
            auto pers = perspective(fov, aspect, near, far);
            STATIC_REQUIRE(std::is_same_v<decltype(pers), mat4f>);
            auto pers_glm = glm::perspective<float>(fov, aspect, near, far);
            REQUIRE(pers == pers_glm);
        }
    }

    SECTION("Chaining method") {
        mat4f mat_t;
        mat4f mat_r;
        mat4f mat_s;
        mat4f mat_combined;

        constexpr vec3f offset{4.F, -8.F, 9.F};
        constexpr vec3f axis{0.4F, 1.F, -4.F};
        constexpr radians angle{4.F};
        constexpr vec3f scale{0.4F, 3.F, 1.4F};

        mat_t.translate(offset);
        mat_r.rotate(axis, angle);
        mat_s.scale(scale);

        const mat4f default_trs = mat_t * mat_r * mat_s;
        const mat4f random = mat_r * mat_s * mat_s * mat_t * mat_r;

        mat_combined.scale(scale).rotate(axis, angle).translate(offset);
        REQUIRE(approx_equal(mat_combined, default_trs));

        mat_combined = mat4f{}.translate(offset).after_rotate(axis, angle).after_scale(scale);
        REQUIRE(approx_equal(mat_combined, default_trs));

        mat_combined = mat4f{}.rotate(axis, angle).translate(offset).scale(scale).scale(scale).rotate(axis, angle);
        REQUIRE(approx_equal(mat_combined, random));

        mat_combined = mat4f{}.scale(scale).scale(scale).after_translate(offset).after_rotate(axis, angle).rotate(axis, angle);
        REQUIRE(approx_equal(mat_combined, random));
    }
}

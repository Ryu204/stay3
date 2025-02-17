#include <iostream>
#include <catch2/catch_all.hpp>
import stay3;

using namespace st;
using Catch::Approx;

namespace {
bool approx_equal(const vec3f &first, const vec3f &second, float epsilon = 1e-6F) {
    return (std::abs(first.x - second.x) < epsilon) && (std::abs(first.y - second.y) < epsilon) && (std::abs(first.z - second.z) < epsilon);
}

bool approx_equal(const quaternionf &first, const quaternionf &second, float epsilon = 1e-6F) {
    return (std::abs(first.angle() - second.angle()) < epsilon) && approx_equal(first.axis(), second.axis(), epsilon);
}

bool approx_equal(const mat4f &first, const mat4f &second, float epsilon = 1e-6f) {
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            std::cout << first[i][j] << ' ';
        }
        std::cout << '\n';
    }
    std::cout << '\n';
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            std::cout << second[i][j] << ' ';
        }
        std::cout << '\n';
    }

    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            if(std::abs(first[i][j] - second[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}
} // namespace

TEST_CASE("Transform - Default initialization") {
    transform tf;

    SECTION("Default values") {
        const auto &pos = tf.position();
        const auto &rot = tf.rotation();
        const auto &scale = tf.scale();

        REQUIRE(approx_equal(pos, vec3f{0.F}));
        REQUIRE(rot.angle() == Approx{0.F});
        REQUIRE(approx_equal(scale, vec3f{1.F}));
    }

    SECTION("Default matrices") {
        REQUIRE(tf.matrix() == mat4f{});
        REQUIRE(tf.inv_matrix() == mat4f{});
    }
}

TEST_CASE("Transform - Basic operations") {
    transform tf;
    constexpr vec3f offset{1.F, 2.F, -3.F};
    constexpr vec3f axis{0.F, 1.F, 0.F};
    constexpr radians angle{PI / 2.F};
    constexpr vec3f scale{1.F, 4.F, 2.F};

    SECTION("Translation") {
        tf.translate(offset);
        REQUIRE(approx_equal(tf.position(), offset));

        tf.translate(offset);
        REQUIRE(approx_equal(tf.position(), offset * 2.F));

        tf.set_position(offset);
        REQUIRE(approx_equal(tf.position(), offset));
    }

    SECTION("Rotation") {
        tf.rotate(axis, angle);
        REQUIRE(approx_equal(tf.rotation().axis(), axis));
        REQUIRE(tf.rotation().angle() == Approx(angle));

        // Test quaternion rotation
        const quaternionf quat(axis, angle);
        tf.set_rotation(quat);
        REQUIRE(approx_equal(tf.rotation(), quat));
    }

    SECTION("Scale") {
        tf.scale(scale);
        REQUIRE(approx_equal(tf.scale(), scale));

        // Test uniform scale
        constexpr auto uniform_scale = 2.F;
        tf.set_scale(uniform_scale);
        REQUIRE(approx_equal(tf.scale(), vec3f{uniform_scale}));
    }
}

TEST_CASE("Transform - Matrix operations") {
    SECTION("Matrix composition") {
        transform tf1;
        transform tf2;

        constexpr vec3f translation{1.F, 2.F, -3.F};
        const quaternionf rotation{{0.F, 1.F, 0.F}, PI / 2.F};

        tf1.translate(translation);
        tf2.rotate(rotation);

        transform combined;
        combined.translate(translation).rotate(rotation);

        REQUIRE(approx_equal(combined.matrix(), tf1.matrix() * tf2.matrix()));
    }

    SECTION("Matrix inverse") {
        transform tf;
        tf.translate({1.F, 2.F, 3.F})
            .rotate({0.F, 1.F, 0.F}, radians(PI / 4.F))
            .scale({2.F, 2.F, 2.F});

        const mat4f &matrix = tf.matrix();
        const mat4f &inv_matrix = tf.inv_matrix();

        // Test that M * M^-1 = I
        REQUIRE(approx_equal(matrix * inv_matrix, mat4f{}));
    }
}
#include <catch2/catch_all.hpp>
import stay3;
import stay3.test_helper;
using namespace st;
using Catch::Approx;

TEST_CASE("Default initialization") {
    transform tf;

    SECTION("Default values") {
        const auto &pos = tf.position();
        const auto &rot = tf.orientation();
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

TEST_CASE("Constructors and operator =") {
    constexpr vec3f position{10.F, 3.F, -5.F};
    const quaternionf orientation{vec3f{vec_up + 2.F * vec_left + 1.5F * vec_back}.normalized(), PI * 0.345F};
    constexpr vec3f scale{1.F, -0.5F, 2.F};

    const transform constructed{position, orientation, scale};

    SECTION("Value constructor") {
        REQUIRE(approx_equal(constructed.position(), position));
        REQUIRE(approx_equal(constructed.orientation(), orientation));
        REQUIRE(approx_equal(constructed.scale(), scale));
    }

    SECTION("Copy constructor") {
        const transform copied{constructed};
        REQUIRE(copied.matrix() == constructed.matrix());
        REQUIRE(copied.inv_matrix() == constructed.inv_matrix());
    }

    SECTION("Operator =") {
        transform copied;
        copied = constructed;
        REQUIRE(copied.matrix() == constructed.matrix());
        REQUIRE(copied.inv_matrix() == constructed.inv_matrix());
    }
}

TEST_CASE("Basic operations") {
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
        REQUIRE(approx_equal(tf.orientation().axis(), axis));
        REQUIRE(tf.orientation().angle() == Approx(angle));

        // Test quaternion rotation
        const quaternionf quat(axis, angle);
        tf.set_orientation(quat);
        REQUIRE(approx_equal(tf.orientation(), quat));
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

TEST_CASE("Matrix operations") {
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

TEST_CASE("Matrix setter") {
    transform tf;
    mat4f mat;

    constexpr vec3f offset{0.5F, -3.F, 4.5F};
    constexpr vec3f axis{9.F, 0.5F, 1.F};
    constexpr radians angle{1.F};
    constexpr vec3f scale{0.5F, 1.F, 8.F};

    mat.scale(scale).rotate(axis, angle).translate(offset);

    tf.set_matrix(mat);

    REQUIRE(approx_equal(tf.position(), offset));
    REQUIRE(approx_equal(tf.scale(), scale));
    REQUIRE(approx_equal(tf.orientation(), quaternionf{axis.normalized(), angle}));
}

TEST_CASE("Combined transform") {
    transform tf;
    mat4f mat;

    SECTION("Matrix arbitrary transform") {
        mat.translate(vec_right).rotate(vec_up, PI / 2);
        tf.set_matrix(mat);
        constexpr vec3f final_pos{0.F, 0.F, -1.F};
        const vec3f final_axis{vec_up};
        constexpr radians final_rot = PI / 2;
        REQUIRE(approx_equal(tf.position(), final_pos));
        REQUIRE(approx_equal(tf.orientation(), quaternionf{final_axis, final_rot}));
    }

    SECTION("Matrix arbitrary transform with multiple operations") {
        mat.translate(vec_right)
            .rotate(vec_up, PI / 2)
            .scale(vec3f{2.F, 2.F, 2.F})
            .rotate(vec_forward, -PI / 4)
            .translate(vec_up);

        tf.set_matrix(mat);

        constexpr vec3f final_pos{0.F, 1.F, -2.F};

        REQUIRE(approx_equal(tf.position(), final_pos));
        REQUIRE(same_orientation(tf.orientation(), quaternionf{vec_up, PI / 2}.rotate(vec_forward, -PI / 4)));
        REQUIRE(approx_equal(tf.scale(), vec3f{2.F, 2.F, 2.F}));
    }
}
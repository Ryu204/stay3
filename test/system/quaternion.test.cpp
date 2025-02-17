#include <catch2/catch_all.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

import stay3.system;
using namespace st;
using Catch::Approx;

TEST_CASE("Quaternion operations", "[quaternion]") {
    SECTION("Default initialization") {
        quaternionf quat{};
        constexpr quaternionf identity{1.F, 0.F, 0.F, 0.F};
        REQUIRE(quat.x == Approx(identity.x));
        REQUIRE(quat.y == Approx(identity.y));
        REQUIRE(quat.z == Approx(identity.z));
        REQUIRE(quat.w == Approx(identity.w));
    }

    SECTION("Axis-angle constructor") {
        constexpr vec3f axis = vec3f{1.F, 0.F, 0.F};
        constexpr radians angle = glm::radians(90.F);
        const quaternionf quat{axis, angle};
        REQUIRE(glm::angle(quat) == Approx(angle));
        REQUIRE(glm::axis(quat).x == Approx(axis.x));
        REQUIRE(glm::axis(quat).y == Approx(axis.y));
    }

    SECTION("Rotation application") {
        quaternionf q1{glm::angleAxis(glm::radians(45.F), vec3f(0.F, 1.F, 0.F))};
        quaternionf q2{glm::angleAxis(glm::radians(45.F), vec3f(0.F, 1.F, 0.F))};

        quaternionf result = q1;
        result.rotate(q2);

        REQUIRE(result == q1 * q2);
    }

    SECTION("Quaternion to matrix conversion") {
        quaternionf quat{glm::angleAxis(glm::radians(90.F), vec3f(0.F, 1.F, 0.F))};
        const auto mat = quat.matrix();
        REQUIRE(mat == glm::mat4_cast(quat));
    }

    SECTION("Angle and axis method") {
        constexpr vec3f axis = vec3f{1.F, 0.F, 0.F};
        constexpr radians angle = glm::radians(90.F);
        const quaternionf quat{axis, angle};
        REQUIRE(quat.angle() == Approx(angle));
        REQUIRE(quat.axis().x == Approx(axis.x));
        REQUIRE(quat.axis().y == Approx(axis.y));
    }
}

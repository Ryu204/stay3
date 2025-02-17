#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

import stay3;

using namespace st;
using Catch::Matchers::WithinAbs;

constexpr float epsilon = 0.0001f;

TEST_CASE("vec2f - construction and initialization") {
    SECTION("Default constructor initializes to zero") {
        vec2f vec{};
        REQUIRE(vec.x == 0.0f);
        REQUIRE(vec.y == 0.0f);
    }

    SECTION("Constructor with components initializes correctly") {
        vec2f vec{1.0f, 2.0f};
        REQUIRE(vec.x == 1.0f);
        REQUIRE(vec.y == 2.0f);
    }

    SECTION("Copy construction initializes correctly") {
        vec2f v1{3.0f, 4.0f};
        vec2f v2{v1};
        REQUIRE(v2.x == 3.0f);
        REQUIRE(v2.y == 4.0f);
    }
}

TEST_CASE("vec2f - magnitude and magnitude_squared") {
    vec2f vec{3.0f, 4.0f};

    SECTION("Magnitude is calculated correctly") {
        REQUIRE_THAT(vec.magnitude(), WithinAbs(5.0f, epsilon));
    }

    SECTION("Magnitude squared is calculated correctly") {
        REQUIRE(vec.magnitude_squared() == 25.0f);
    }
}

TEST_CASE("vec2f - dot product") {
    vec2f v1{1.0f, 2.0f};
    vec2f v2{3.0f, 4.0f};

    SECTION("Dot product is calculated correctly") {
        REQUIRE(v1.dot(v2) == 11.0f);
    }
}

TEST_CASE("vec4f - construction and initialization") {
    SECTION("Default constructor initializes to zero") {
        vec4f vec{};
        REQUIRE(vec.x == 0.0f);
        REQUIRE(vec.y == 0.0f);
        REQUIRE(vec.z == 0.0f);
        REQUIRE(vec.w == 0.0f);
    }

    SECTION("Constructor with components initializes correctly") {
        vec4f vec{1.0f, 2.0f, 3.0f, 4.0f};
        REQUIRE(vec.x == 1.0f);
        REQUIRE(vec.y == 2.0f);
        REQUIRE(vec.z == 3.0f);
        REQUIRE(vec.w == 4.0f);
    }

    SECTION("Copy construction initializes correctly") {
        vec4f v1{5.0f, 6.0f, 7.0f, 8.0f};
        vec4f v2{v1};
        REQUIRE(v2.x == 5.0f);
        REQUIRE(v2.y == 6.0f);
        REQUIRE(v2.z == 7.0f);
        REQUIRE(v2.w == 8.0f);
    }
}

TEST_CASE("vec4f - magnitude and magnitude_squared") {
    vec4f vec{1.0f, 2.0f, 2.0f, 0.0f};

    SECTION("Magnitude is calculated correctly") {
        REQUIRE_THAT(vec.magnitude(), WithinAbs(3.0f, epsilon));
    }

    SECTION("Magnitude squared is calculated correctly") {
        REQUIRE(vec.magnitude_squared() == 9.0f);
    }
}

TEST_CASE("vec4f - dot product") {
    vec4f v1{1.0f, 2.0f, 3.0f, 4.0f};
    vec4f v2{4.0f, 3.0f, 2.0f, 1.0f};

    SECTION("Dot product is calculated correctly") {
        REQUIRE(v1.dot(v2) == 20.0f);
    }
}

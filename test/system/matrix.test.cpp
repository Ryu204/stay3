#include <catch2/catch_all.hpp>
#include <glm/glm.hpp>
import stay3;

TEST_CASE("Matrix operations", "[mat]") {
    using namespace st;

    SECTION("Matrix initialization should return identity") {
        constexpr mat4f identity{};
        REQUIRE(identity == glm::mat4{1.F});
    }

    SECTION("Matrix construction") {
        constexpr mat4f identity(3.F);
        REQUIRE(identity == glm::mat4{3.F});
    }

    SECTION("Matrix inversion") {
        constexpr mat4f matrix = glm::mat4{2.F};
        const mat4f inv_matrix = matrix.inv();
        REQUIRE(glm::inverse(matrix) == inv_matrix);
    }

    SECTION("Matrix multiplication") {
        mat4f mata{1.F};
        constexpr mat4f matb{2.F};
        const mat4f result = mata * matb;
        mata *= matb;
        REQUIRE(mata == result);
    }
}

#include <catch2/catch_all.hpp>

import stay3.core;

using namespace st;
using Catch::Approx;
using Catch::Matchers::Predicate;

TEST_CASE("Generates valid colors") {
    constexpr auto check_count = 100;
    constexpr auto is_color_xyz = [](float val) { return val <= 1.F && val >= 0.F; };
    for(int i = 0; i < check_count; ++i) {
        vec4f color = random_color();
        REQUIRE_THAT(color.x, Predicate<float>(is_color_xyz));
        REQUIRE_THAT(color.y, Predicate<float>(is_color_xyz));
        REQUIRE_THAT(color.z, Predicate<float>(is_color_xyz));
        REQUIRE(color.w == Approx{1.0f});
    }
}
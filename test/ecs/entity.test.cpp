#include <cstdint>
#include <limits>
#include <catch2/catch_all.hpp>
import stay3;

using namespace st;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Default init") {
    entity en;
    REQUIRE(en.is_null());
}

TEST_CASE("Conversion from/to numeric") {
    constexpr std::uint8_t small = 56;
    constexpr std::size_t big = std::numeric_limits<std::size_t>::max();

    constexpr auto test = [](auto num) {
        auto new_en = entity::from_numeric(num);
        if(new_en.is_null()) { return; }
        REQUIRE(new_en.numeric() == num);
    };

    test(small);
    test(big);
}
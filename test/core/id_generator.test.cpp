#include <cstdint>
#include <limits>
#include <unordered_set>
#include <catch2/catch_all.hpp>
import stay3;

using namespace st;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("id_generator basic functionality") {
    id_generator<unsigned int> generator;

    SECTION("IDs are generated sequentially") {
        REQUIRE(generator.create() == 0);
        REQUIRE(generator.create() == 1);
        REQUIRE(generator.create() == 2);
    }

    SECTION("Recycling an ID makes it available again") {
        auto id1 = generator.create();
        generator.recycle(id1);
        REQUIRE(generator.create() == id1);
    }

    SECTION("Has enough ID as type allows") {
        using type = std::uint16_t;
        constexpr auto mx = std::numeric_limits<type>::max();
        id_generator<type> short_generator;

        std::unordered_set<type> created_ids;
        for(type i = 0; i < mx; ++i) {
            created_ids.insert(short_generator.create());
        }
        REQUIRE(created_ids.size() == mx);
    }

    SECTION("Created, unrecycled id is active") {
        const auto id1 = generator.create();
        const auto id2 = generator.create();
        const auto id3 = generator.create();

        generator.recycle(id1);
        REQUIRE(!generator.is_id_active(id1));
        REQUIRE(generator.is_id_active(id2));
        REQUIRE(generator.is_id_active(id3));
    }
}

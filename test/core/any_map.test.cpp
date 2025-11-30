#include <string>
#include <catch2/catch_test_macros.hpp>

import stay3.core;

TEST_CASE("any_map basic functionality", "[any_map]") {
    st::any_map map;

    SECTION("emplace and get basic types") {
        auto &int_val = map.emplace<int>(42);
        auto &str_val = map.emplace<std::string>("test");

        REQUIRE(int_val == 42);
        REQUIRE(str_val == "test");

        REQUIRE(map.get<int>() == 42);
        REQUIRE(map.get<std::string>() == "test");
    }

    SECTION("erase and clear") {
        map.emplace<int>(42);
        map.emplace<float>(3.14f);

        map.erase<int>();
        REQUIRE_NOTHROW(map.get<float>());

        map.clear();
    }
}

TEST_CASE("any_map named entries", "[any_map]") {
    st::any_map map;
    enum class test_key : std::uint8_t {
        first = 1,
        second = 2,
    };

    SECTION("emplace_as and get with keys") {
        auto &int_val = map.emplace_as<int>(test_key::first, 10);
        auto &str_val = map.emplace_as<std::string>(test_key::second, "test");

        REQUIRE(int_val == 10);
        REQUIRE(str_val == "test");

        REQUIRE(map.get<int>(test_key::first) == 10);
        REQUIRE(map.get<std::string>(test_key::second) == "test");
    }

    SECTION("erase named entries") {
        map.emplace_as<int>(test_key::first, 10);
        map.emplace_as<float>(test_key::first, 1.5f);

        map.erase<int>(test_key::first);
        REQUIRE_THROWS_AS(map.get<int>(test_key::first), std::out_of_range);
        REQUIRE_NOTHROW(map.get<float>(test_key::first));
    }

    SECTION("same type with different keys") {
        map.emplace_as<int>(test_key::first, 10);
        map.emplace_as<int>(test_key::second, 20);

        REQUIRE(map.get<int>(test_key::first) == 10);
        REQUIRE(map.get<int>(test_key::second) == 20);
    }
}

TEST_CASE("any_map modification", "[any_map]") {
    st::any_map map;

    SECTION("modify after get") {
        map.emplace<std::vector<int>>(3, 10);

        auto &vec = map.get<std::vector<int>>();
        vec.push_back(20);

        REQUIRE(map.get<std::vector<int>>().size() == 4);
        REQUIRE(map.get<std::vector<int>>()[3] == 20);
    }
}
#include <optional>
#include <string>
#include <catch2/catch_all.hpp>

import stay3.core;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::MessageMatches;
using st::read_file_as_str;
using st::read_file_as_str_nothrow;

TEST_CASE("File not found") {
    REQUIRE_THROWS_MATCHES(read_file_as_str("non_existent"), st::file_error, MessageMatches(ContainsSubstring("Failed to open")));
    REQUIRE_NOTHROW(read_file_as_str_nothrow("non_existent"));
    REQUIRE_THROWS_MATCHES(read_file_as_str("test"), st::file_error, MessageMatches(ContainsSubstring("Failed to open")));
}

TEST_CASE("Empty file [throw version]") {
    std::string result;
    REQUIRE_NOTHROW(result = read_file_as_str("./assets/empty_file"));
    REQUIRE(result == "");
}

TEST_CASE("Empty file [nothrow version]") {
    std::optional<std::string> result;
    REQUIRE_NOTHROW(result = read_file_as_str_nothrow("./assets/empty_file"));
    REQUIRE(result.has_value());
    REQUIRE(result.value() == "");
}

TEST_CASE("Non empty file [throw version]") {
    std::string result;
    REQUIRE_NOTHROW(result = read_file_as_str("./assets/123.txt"));
    REQUIRE(result == "123");
}

TEST_CASE("Non empty file [nothrow version]") {
    std::optional<std::string> result;
    REQUIRE_NOTHROW(result = read_file_as_str_nothrow("./assets/123.txt"));
    CHECK(result.has_value());
    REQUIRE(result.value() == "123");
}

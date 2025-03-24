#include <string>
#include <catch2/catch_all.hpp>

import stay3.input;
using namespace st;

TEST_CASE("get name") {
#define CHECK_KEY_NAME(code, name) REQUIRE(std::string{get_name(scancode::code)} == (name));

    SECTION("Alphanumerics") {
        CHECK_KEY_NAME(a, "a");
        CHECK_KEY_NAME(s, "s");
        CHECK_KEY_NAME(w, "w");
        CHECK_KEY_NAME(q, "q");
        CHECK_KEY_NAME(e, "e");
        CHECK_KEY_NAME(d, "d");
        CHECK_KEY_NAME(num_1, "1");
        CHECK_KEY_NAME(num_2, "2");
        CHECK_KEY_NAME(num_3, "3");
    }

    SECTION("Arrows") {
        CHECK_KEY_NAME(up, "up");
        CHECK_KEY_NAME(down, "down");
        CHECK_KEY_NAME(left, "left");
        CHECK_KEY_NAME(right, "right");
    }

    SECTION("Specials") {
        CHECK_KEY_NAME(enter, "enter");
        CHECK_KEY_NAME(tab, "tab");
        CHECK_KEY_NAME(esc, "esc");
        CHECK_KEY_NAME(space, "space");
    }
#undef CHECK_KEY_NAME

    SECTION("Unlabled enum") {
        REQUIRE(get_name(static_cast<scancode>(99999)) == "undefined");
    }
}
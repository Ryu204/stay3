#include <utility>
#include <catch2/catch_test_macros.hpp>
import stay3;

using namespace st;

TEST_CASE("glfw_window behavior") {
    glfw_window window1;

    SECTION("Poll event - default") {
        REQUIRE(window1.poll_event().is<event::none>());
    }

    SECTION("Move semantics - constructor") {
        REQUIRE(window1.is_open());

        auto window2 = std::move(window1);
        REQUIRE(window2.is_open());
        REQUIRE_FALSE(window1.is_open());
    }

    SECTION("Close behavior") {
        REQUIRE(window1.is_open());
        window1.close();
        REQUIRE_FALSE(window1.is_open());
    }
}

#ifndef __EMSCRIPTEN__
// Currently, Emscripten's GLFW does not support multiple windows
TEST_CASE("Native multiple windows") {
    glfw_window window1;

    SECTION("Creation and destruction") {
        REQUIRE_NOTHROW(glfw_window{});
    }

    SECTION("Move semantics - assignment") {
        REQUIRE(window1.is_open());

        glfw_window window2;
        REQUIRE(window2.is_open());
        window2 = std::move(window1);
        REQUIRE_FALSE(window1.is_open());
        REQUIRE(window2.is_open());
    }

    SECTION("Multiple windows coexistence") {
        glfw_window window2;
        REQUIRE(window1.is_open());
        REQUIRE(window2.is_open());
    }
}
#endif

TEST_CASE("keyboard query") {
    // We cannot test much here
    st::glfw_window window;

    REQUIRE(window.get_key(scancode::a) == key_status::released);
    REQUIRE(window.get_key(scancode::w) == key_status::released);
    REQUIRE(window.get_key(scancode::d) == key_status::released);
    REQUIRE(window.get_key(scancode::up) == key_status::released);
    REQUIRE(window.get_key(scancode::down) == key_status::released);
}

#include <catch2/catch_all.hpp>
import stay3;
import stay3.test_helper;

using Catch::Approx;

TEST_CASE("Camera operations", "[camera]") {
    using namespace st;

    SECTION("Default camera initialization") {
        camera cam{};
        REQUIRE(cam.near == camera::default_near);
        REQUIRE(cam.far == camera::default_far);
        REQUIRE(cam.clear_color == camera::default_background_color);
        REQUIRE(cam.type() == camera_type::perspective);
        REQUIRE(cam.perspective().fov == camera::perspective_data::default_fov);
    }

    SECTION("Perspective camera modification") {
        camera cam{};
        constexpr auto fov = 90 * PI / 180;
        cam.data = camera::perspective_data{.fov = fov};
        REQUIRE(cam.perspective().fov == Approx{fov});
        REQUIRE(cam.type() == camera_type::perspective);
    }

    SECTION("Orthographic camera initialization") {
        camera cam{
            .data = camera::orthographic_data{.width = 15.F},
        };
        REQUIRE(cam.type() == camera_type::orthographic);
        REQUIRE(cam.orthographic().width == 15.F);
    }
}

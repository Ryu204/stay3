#include <catch2/catch_all.hpp>
import stay3;
import stay3.test_helper;
using namespace st;
using Catch::Approx;

TEST_CASE("Constructions and destruction of multiple worlds at same time") {
    tree_context dummy_1;
    tree_context dummy_2;

    SECTION("Single") {
        physics_world wld{dummy_1};
    }

    SECTION("Double") {
        // Lifetime: 1ctor -> 2ctor -> 1dtor -> 2dtor
        std::shared_ptr<physics_world> wld_2_ptr;
        {
            physics_world wld_1{dummy_1};
            wld_2_ptr = std::make_shared<physics_world>(dummy_2);
        }
    }
}

TEST_CASE("World state behavior") {
    tree_context dummy_1;
    physics_world wld{dummy_1};
    constexpr vec3f position = vec_up + vec_left + vec_back;
    quaternionf orientation;
    constexpr auto default_time = 30;

    constexpr auto advance = [](auto &world) {
        for(auto i = 0; i < default_time; ++i) {
            world.update(1.F);
        }
    };
    const auto new_entity = [&dummy_1] {
        return dummy_1.ecs().create();
    };

    auto dynamic_id = wld.create(collider::info{box{1, 1, 1}}, position, orientation, rigidbody::dynamic, new_entity());
    advance(wld);

    SECTION("New single dynamic body is in change list") {
        const auto &change_list = wld.bodies_with_changed_state();
        REQUIRE(change_list.size() == 1);
        REQUIRE(*change_list.begin() == dynamic_id);

        SECTION("Static body will never be in change list") {
            wld.create(collider::info{box{10, 10, 10}}, position, orientation, rigidbody::fixed, new_entity());
            advance(wld);
            REQUIRE(change_list.size() == 1);
            REQUIRE(*change_list.begin() == dynamic_id);
        }
    }

    SECTION("Transform setter/getter") {
        wld.set_transform(dynamic_id, position, orientation);
        auto tf = wld.transform(dynamic_id);
        REQUIRE(approx_equal(tf.position(), position));
        REQUIRE(approx_equal_orientation(tf.orientation(), orientation));
    }

    SECTION("Freefall rigidbody") {
        auto tf = wld.transform(dynamic_id);
        constexpr auto predicted_pos = position + default_time * 1.F * default_time / 2.F * physics_config::earth_gravity;
        // Predicted pos is only the correct one if there is no drag
        REQUIRE(predicted_pos.y < tf.position().y);
        REQUIRE(predicted_pos.x == Approx(tf.position().x));
        REQUIRE(predicted_pos.z == Approx(tf.position().z));
    }

    SECTION("Zero gravity") {
        physics_world wld{dummy_1, {.gravity = {}}};
        auto body = wld.create(collider::info{box{1, 1, 1}}, position, orientation, rigidbody::dynamic, new_entity());
        advance(wld);
        REQUIRE(approx_equal(wld.transform(body).position(), position));
    }

    SECTION("Entity data") {
        auto en = new_entity();
        auto id = wld.create(collider::info{box{1, 1, 1}}, position, orientation, rigidbody::fixed, en);
        REQUIRE(wld.entity(id) == en);
    }

    SECTION("Destruction does not throw") {
        REQUIRE_NOTHROW(wld.destroy(dynamic_id));
        SECTION("World is still functional") {
            REQUIRE_NOTHROW(advance(wld));
        }
    }

    SECTION("Render does not throw") {
        auto camera_en = new_entity();
        dummy_1.ecs().emplace<main_camera>(camera_en);
        dummy_1.ecs().emplace<camera>(camera_en);
        // Mock global transform. In reality transform sync system will add it
        dummy_1.ecs().emplace<global_transform>(camera_en);
        physics_world wld{dummy_1, {.debug_draw = true}};
        REQUIRE_NOTHROW(wld.render());
    }
}
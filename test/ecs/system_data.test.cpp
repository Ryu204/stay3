#include <catch2/catch_all.hpp>

import stay3.ecs;
import stay3.input;

using namespace st;

class context {};

class valid_update_system {
public:
    void update(short, const context &) {}
};

class valid_post_update_system {
public:
    [[nodiscard]] static bool post_update(bool, const context &) {
        return false;
    }
};

class valid_start_system {
public:
    void start(context &) {}
};

class valid_cleanup_system {
public:
    void cleanup(const context &) {}
};

class valid_render_system {
public:
    static bool render(const context &) {
        return true;
    }
};

class valid_input_system {
public:
    static std::string input(const event &, const context &) {
        return "123";
    }
};

class mixed_system {
public:
    void update(double, const context &) {}
    void post_update(double, const context &) {}
    void start(const context &) {}
    void cleanup(context &) {}
    void render(context &) {}
    void input(event, context &) {}
};

class update_and_render {
public:
    void update(long, const context &) {}
    static int render(context &) {
        return 0;
    }

private:
    void cleanup(context &) {}
};

TEST_CASE("Concepts work correctly") {
    STATIC_REQUIRE(is_update_system<valid_update_system, context>);
    STATIC_REQUIRE(is_post_update_system<valid_post_update_system, context>);
    STATIC_REQUIRE(is_start_system<valid_start_system, context>);
    STATIC_REQUIRE(is_cleanup_system<valid_cleanup_system, context>);
    STATIC_REQUIRE(is_render_system<valid_render_system, context>);
    STATIC_REQUIRE(is_input_system<valid_input_system, context>);

    STATIC_REQUIRE(is_update_system<mixed_system, context>);
    STATIC_REQUIRE(is_post_update_system<mixed_system, context>);
    STATIC_REQUIRE(is_start_system<mixed_system, context>);
    STATIC_REQUIRE(is_cleanup_system<mixed_system, context>);
    STATIC_REQUIRE(is_render_system<mixed_system, context>);
    STATIC_REQUIRE(is_input_system<mixed_system, context>);

    STATIC_REQUIRE(is_update_system<update_and_render, context>);
    STATIC_REQUIRE(is_render_system<update_and_render, context>);
    STATIC_REQUIRE_FALSE(is_start_system<update_and_render, context>);
    STATIC_REQUIRE_FALSE(is_cleanup_system<update_and_render, context>);
}

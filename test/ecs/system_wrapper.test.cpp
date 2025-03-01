#include <utility>

#include <catch2/catch_all.hpp>

import stay3.ecs;
import stay3.system;

using namespace st;

class test_context {};

class update_system {
public:
    update_system(bool *called = nullptr, seconds *last_delta = nullptr)
        : called{called}, last_delta{last_delta} {}
    bool *called;
    seconds *last_delta;
    void update(seconds delta, test_context &) const {
        if(called != nullptr) { *called = true; }
        if(last_delta != nullptr) { *last_delta = delta; }
    }
};

class start_system {
public:
    start_system(bool *called = nullptr)
        : called{called} {}
    bool *called;
    void start(test_context &) const {
        if(called != nullptr) { *called = true; }
    }
};

class cleanup_system {
public:
    cleanup_system(bool *called = nullptr)
        : called{called} {}
    bool *called;
    void cleanup(test_context &) const {
        if(called != nullptr) { *called = true; }
    }
};

class render_system {
public:
    render_system(bool *called = nullptr)
        : called{called} {}
    bool *called;
    void render(test_context &) const {
        if(called != nullptr) { *called = true; }
    }
};

class mixed_system {
public:
    // NOLINTNEXTLINE
    mixed_system(bool *update_called = nullptr, bool *start_called = nullptr,
                 bool *cleanup_called = nullptr, bool *render_called = nullptr)
        : update_called{update_called}, start_called{start_called},
          cleanup_called{cleanup_called}, render_called{render_called} {}
    bool *update_called;
    bool *start_called;
    bool *cleanup_called;
    bool *render_called;

    void update(seconds, test_context &) const {
        if(update_called != nullptr) { *update_called = true; }
    }
    void start(test_context &) const {
        if(start_called != nullptr) { *start_called = true; }
    }
    void cleanup(test_context &) const {
        if(cleanup_called != nullptr) { *cleanup_called = true; }
    }
    void render(test_context &) const {
        if(render_called != nullptr) { *render_called = true; }
    }
};

TEST_CASE("System Wrapper correctly registers callable methods") {
    bool called{false};
    seconds last_delta{0.F};

    system_wrapper<test_context> update_sys{std::in_place_type<update_system>, update_system{&called, &last_delta}};
    REQUIRE(update_sys.is_type<sys_type::update>());
    REQUIRE_FALSE(update_sys.is_type<sys_type::start>());
    REQUIRE_FALSE(update_sys.is_type<sys_type::cleanup>());
    REQUIRE_FALSE(update_sys.is_type<sys_type::render>());

    start_system start{&called};
    system_wrapper<test_context> start_sys{std::in_place_type<start_system>, start};
    REQUIRE(start_sys.is_type<sys_type::start>());
    REQUIRE_FALSE(start_sys.is_type<sys_type::update>());
    REQUIRE_FALSE(start_sys.is_type<sys_type::cleanup>());
    REQUIRE_FALSE(start_sys.is_type<sys_type::render>());

    const cleanup_system cleanup{};
    system_wrapper<test_context> cleanup_sys{std::in_place_type<cleanup_system>, cleanup};
    REQUIRE(cleanup_sys.is_type<sys_type::cleanup>());
    REQUIRE_FALSE(cleanup_sys.is_type<sys_type::update>());
    REQUIRE_FALSE(cleanup_sys.is_type<sys_type::start>());
    REQUIRE_FALSE(cleanup_sys.is_type<sys_type::render>());

    system_wrapper<test_context> render_sys{std::in_place_type<render_system>, &called};
    REQUIRE(render_sys.is_type<sys_type::render>());
    REQUIRE_FALSE(render_sys.is_type<sys_type::update>());
    REQUIRE_FALSE(render_sys.is_type<sys_type::start>());
    REQUIRE_FALSE(render_sys.is_type<sys_type::cleanup>());

    SECTION("Mixed systems") {
        system_wrapper<test_context> mixed_sys{std::in_place_type<mixed_system>, mixed_system{&called, &called, &called, &called}};
        REQUIRE(mixed_sys.is_type<sys_type::update>());
        REQUIRE(mixed_sys.is_type<sys_type::start>());
        REQUIRE(mixed_sys.is_type<sys_type::cleanup>());
        REQUIRE(mixed_sys.is_type<sys_type::render>());
    }
}

TEST_CASE("System Wrapper executes the correct function") {
    test_context ctx;

    bool update_called{false};
    seconds last_delta{0.F};
    system_wrapper<test_context> update_sys{std::in_place_type<update_system>, &update_called, &last_delta};
    update_sys.call_method<sys_type::update>(seconds{3.F}, ctx);
    REQUIRE(update_called);
    REQUIRE(last_delta == seconds{3.F});

    bool start_called{false};
    system_wrapper<test_context> start_sys{std::in_place_type<start_system>, &start_called};
    start_sys.call_method<sys_type::start>(ctx);
    REQUIRE(start_called);

    bool cleanup_called{false};
    system_wrapper<test_context> cleanup_sys{std::in_place_type<cleanup_system>, &cleanup_called};
    cleanup_sys.call_method<sys_type::cleanup>(ctx);
    REQUIRE(cleanup_called);

    bool render_called{false};
    system_wrapper<test_context> render_sys{std::in_place_type<render_system>, &render_called};
    render_sys.call_method<sys_type::render>(ctx);
    REQUIRE(render_called);
}

TEST_CASE("System Wrapper handles multiple functions correctly") {
    test_context ctx;
    bool update_called{false};
    bool start_called{false};
    bool cleanup_called{false};
    bool render_called{false};
    system_wrapper<test_context> mixed_sys{std::in_place_type<mixed_system>, &update_called, &start_called, &cleanup_called, &render_called};

    mixed_sys.call_method<sys_type::update>(seconds{2.5}, ctx);
    mixed_sys.call_method<sys_type::start>(ctx);
    mixed_sys.call_method<sys_type::cleanup>(ctx);
    mixed_sys.call_method<sys_type::render>(ctx);

    REQUIRE(mixed_sys.is_type<sys_type::update>());
    REQUIRE(mixed_sys.is_type<sys_type::start>());
    REQUIRE(mixed_sys.is_type<sys_type::cleanup>());
    REQUIRE(mixed_sys.is_type<sys_type::render>());

    REQUIRE(update_called);
    REQUIRE(start_called);
    REQUIRE(cleanup_called);
    REQUIRE(render_called);
}
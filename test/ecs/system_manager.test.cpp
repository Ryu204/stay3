#include <string>
#include <type_traits>
#include <catch2/catch_all.hpp>

import stay3.core;
import stay3.ecs;
import stay3.input;

using namespace st;
using Catch::Matchers::RangeEquals;

struct test_context {
    int start_count{};
    int update_count{};
    int cleanup_count{};
    int render_count{};
    int post_update_count{};
    int input_count{};

    std::vector<std::string> messages;
};

struct render_system {
    render_system(std::string name)
        : name{std::move(name)} {}
    void render(test_context &ctx) const {
        ctx.render_count++;
        ctx.messages.emplace_back("render " + name);
    }
    std::string name;
};

struct update_and_cleanup_system {
    static void update(float, test_context &ctx) {
        ctx.update_count++;
        ctx.messages.emplace_back("update cleanup");
    }
    static void cleanup(test_context &ctx) {
        ctx.cleanup_count++;
        ctx.messages.emplace_back("update cleanup");
    }
};

struct start_system {
    static void update(float, test_context &&) {
        // This is not valid update syntax
    }
    static void start(test_context &ctx) {
        ctx.start_count++;
        ctx.messages.emplace_back("start");
    }
};

struct post_update_system {
    static bool post_update(int, test_context &ctx) {
        ctx.post_update_count++;
        ctx.messages.emplace_back("post update");
        return false;
    }
};

struct input_system {
    static sys_run_result input(event, test_context &ctx) {
        ctx.input_count++;
        ctx.messages.emplace_back("input");
        return sys_run_result::noop;
    }
};

TEST_CASE("Added systems does not run automatically") {
    test_context ctx;
    system_manager<test_context> manager;

    manager.add<start_system>();
    manager.add<render_system>("webgpu");
    manager.add<update_and_cleanup_system>();
    manager.add<post_update_system>();
    manager.add<input_system>();

    manager.start(ctx);
    manager.input(event::close_requested{}, ctx);
    manager.update(seconds{1.F}, ctx);
    manager.post_update(seconds{1.F}, ctx);
    manager.render(ctx);
    manager.cleanup(ctx);

    REQUIRE(ctx.start_count == 0);
    REQUIRE(ctx.update_count == 0);
    REQUIRE(ctx.post_update_count == 0);
    REQUIRE(ctx.render_count == 0);
    REQUIRE(ctx.cleanup_count == 0);
    REQUIRE(ctx.input_count == 0);
    REQUIRE(ctx.messages.empty());
}

TEST_CASE("Added system run when registered") {
    test_context ctx;
    system_manager<test_context> manager;

    manager
        .add<start_system>()
        .run_as<sys_type::start>();
    manager
        .add<render_system>("webgpu")
        .run_as<sys_type::render>();
    manager
        .add<update_and_cleanup_system>()
        .run_as<sys_type::update>()
        .run_as<sys_type::cleanup>();
    manager
        .add<post_update_system>()
        .run_as<sys_type::post_update>();
    manager
        .add<input_system>()
        .run_as<sys_type::input>();

    manager.start(ctx);
    manager.input(event::close_requested{}, ctx);
    manager.update(seconds{1.F}, ctx);
    manager.post_update(seconds{1.F}, ctx);
    manager.render(ctx);
    manager.cleanup(ctx);

    REQUIRE(ctx.start_count == 1);
    REQUIRE(ctx.update_count == 1);
    REQUIRE(ctx.post_update_count == 1);
    REQUIRE(ctx.render_count == 1);
    REQUIRE(ctx.cleanup_count == 1);
    REQUIRE(ctx.input_count == 1);
    REQUIRE_THAT(ctx.messages, RangeEquals({"start", "input", "update cleanup", "post update", "render webgpu", "update cleanup"}));
}

TEST_CASE("Added system run with priority") {
    test_context ctx;
    system_manager<test_context> manager;
    manager
        .add<render_system>("first")
        .run_as<sys_type::render>(sys_priority::medium);
    manager
        .add<render_system>("fourth")
        .run_as<sys_type::render>(static_cast<std::underlying_type_t<sys_priority>>(sys_priority::lowest));
    manager
        .add<render_system>("third")
        .run_as<sys_type::render>(sys_priority::very_low);
    manager
        .add<render_system>("second")
        .run_as<sys_type::render>(static_cast<std::underlying_type_t<sys_priority>>(sys_priority::low));
    manager.render(ctx);

    REQUIRE(ctx.render_count == 4);
    REQUIRE_THAT(ctx.messages, RangeEquals({"render first", "render second", "render third", "render fourth"}));
}

TEST_CASE("System is instantiated once") {
    struct my_system {
        int id{};
        void start(test_context &) {
            id = 1;
        }
        void update(seconds, test_context &) const {
            if(id == 0) {
                throw std::runtime_error{"Should be 1"};
            }
        }
    };
    test_context ctx;
    system_manager<test_context> manager;
    manager
        .add<my_system>()
        .run_as<sys_type::start>()
        .run_as<sys_type::update>();
    manager.start(ctx);
    REQUIRE_NOTHROW(manager.update(1.F, ctx));
}

TEST_CASE("System run result is used") {
    test_context ctx;
    system_manager<test_context> manager;

    struct convertible_to_run_result {
        bool exit{};
        operator sys_run_result() const {
            return exit ? sys_run_result::exit : sys_run_result::noop;
        }
    };
    STATIC_REQUIRE(std::is_convertible_v<convertible_to_run_result, sys_run_result>);
    using first_system = render_system;
    struct exiting_system {
        static convertible_to_run_result render(test_context &ctx) {
            ctx.messages.emplace_back("exit");
            ++ctx.render_count;
            return {.exit = true};
        }
    };
    using third_system = render_system;

    manager
        .add<first_system>("software")
        .run_as<sys_type::render>(sys_priority::high);
    manager
        .add<exiting_system>()
        .run_as<sys_type::render>(sys_priority::medium);
    manager
        .add<third_system>("dedicated gpu")
        .run_as<sys_type::render>(sys_priority::low);
    manager.render(ctx);

    REQUIRE(ctx.render_count == 2);
    REQUIRE_THAT(ctx.messages, RangeEquals({"render software", "exit"}));
}
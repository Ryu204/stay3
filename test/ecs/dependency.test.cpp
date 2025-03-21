#include <type_traits>
#include <catch2/catch_all.hpp>

import stay3.ecs;

using namespace st;

struct non_default_constructible {
    non_default_constructible(int val)
        : val{val} {}
    int val;
};
static_assert(!std::is_default_constructible_v<non_default_constructible>);

using comp_first = int;
using comp_second = float;
using comp_third = non_default_constructible;

TEST_CASE("Hard dependency") {
    ecs_registry reg;
    const auto en = reg.create_entity();

    SECTION("Default constructible") {
        make_hard_dependency<comp_first, comp_second>(reg);
        REQUIRE_FALSE(reg.has_components<comp_second>(en));
        reg.add_component<comp_second>(en);
        REQUIRE(reg.has_components<comp_first>(en));
        reg.remove_component<comp_second>(en);
        REQUIRE_FALSE(reg.has_components<comp_first>(en));
    }

    SECTION("Non default constructible") {
        constexpr auto value = 4;
        make_hard_dependency<comp_third, comp_second>(reg, value);
        REQUIRE_FALSE(reg.has_components<comp_second>(en));
        reg.add_component<comp_second>(en);
        REQUIRE(reg.has_components<comp_third>(en));
        REQUIRE(reg.get_components<comp_third>(en)->val == value);
        reg.remove_component<comp_second>(en);
        REQUIRE_FALSE(reg.has_components<comp_third>(en));
    }
}

TEST_CASE("Soft dependency") {
    ecs_registry reg;
    const auto en = reg.create_entity();
    SECTION("Deps does not exist before Base") {
        SECTION("Default constructible") {
            make_soft_dependency<comp_first, comp_second>(reg);
            REQUIRE_FALSE(reg.has_components<comp_second>(en));
            reg.add_component<comp_second>(en);
            REQUIRE(reg.has_components<comp_first>(en));
            reg.remove_component<comp_second>(en);
            REQUIRE_FALSE(reg.has_components<comp_first>(en));
        }

        SECTION("Non default constructible") {
            constexpr auto value = 4;
            make_soft_dependency<comp_third, comp_second>(reg, value);
            REQUIRE_FALSE(reg.has_components<comp_second>(en));
            reg.add_component<comp_second>(en);
            REQUIRE(reg.has_components<comp_third>(en));
            REQUIRE(reg.get_components<comp_third>(en)->val == value);
            reg.remove_component<comp_second>(en);
            REQUIRE_FALSE(reg.has_components<comp_third>(en));
        }
    }

    SECTION("Deps existed before Base (not a dependency)") {
        SECTION("Default constructible") {
            constexpr auto value = 4;
            make_soft_dependency<comp_first, comp_second>(reg);
            reg.add_component<comp_first>(en, value);
            REQUIRE_FALSE(reg.has_components<comp_second>(en));
            reg.add_component<comp_second>(en);
            REQUIRE(reg.has_components<comp_first>(en));
            REQUIRE(*reg.get_components<const comp_first>(en) == value);
            reg.remove_component<comp_second>(en);
            REQUIRE(reg.has_components<comp_first>(en));
            REQUIRE(*reg.get_components<const comp_first>(en) == value);
        }

        SECTION("Non default constructible") {
            constexpr auto value_make_deps = 4;
            constexpr auto value_add = 6;
            make_soft_dependency<comp_third, comp_second>(reg, value_make_deps);
            reg.add_component<comp_third>(en, value_add);
            REQUIRE_FALSE(reg.has_components<comp_second>(en));
            reg.add_component<comp_second>(en);
            REQUIRE(reg.has_components<comp_third>(en));
            REQUIRE(reg.get_components<comp_third>(en)->val == value_add);
            reg.remove_component<comp_second>(en);
            REQUIRE(reg.has_components<comp_third>(en));
            REQUIRE(reg.get_components<const comp_third>(en)->val == value_add);
        }
    }
}

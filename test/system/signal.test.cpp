#include <catch2/catch_all.hpp>

import stay3.system;

using st::signal;
using st::sink;

namespace {
using signature = void(int &);

void twice(int &val) {
    val *= 2;
}

struct adder {
    int amount{};
    void add(int &val) const {
        val = amount + val;
    }
};
} // namespace

TEST_CASE("Signal is correctly published") {
    signal<signature> sn;
    sink sk{sn};
    int value = 1;

    SECTION("Free function") {
        sk.connect<&twice>();
        sn.publish(value);
        REQUIRE(value == 2);
        sk.disconnect<&twice>();
        sn.publish(value);
        REQUIRE(value == 2);
    }

    SECTION("Functor") {
        adder ad{5};
        sk.connect<&adder::add>(ad);
        sn.publish(value);
        REQUIRE(value == 6);
        sk.disconnect<&adder::add>(ad);
        sn.publish(value);
        REQUIRE(value == 6);
    }
}
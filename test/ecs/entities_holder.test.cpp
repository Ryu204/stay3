#include <tuple>
#include <unordered_set>
#include <catch2/catch_all.hpp>

import stay3.ecs;

using namespace st;

TEST_CASE("Entity manipulation") {
    ecs_registry reg;
    entities_holder es{reg};

    SECTION("Push and erase") {
        const auto en = es.push();
        REQUIRE(reg.contains_entity(en));

        const auto en2 = es.push();
        es.erase(1);
        REQUIRE_FALSE(reg.contains_entity(en2));
        REQUIRE(reg.contains_entity(en));

        es.erase(0);
        REQUIRE_FALSE(reg.contains_entity(en));
    }

    SECTION("Size and empty check") {
        REQUIRE(es.is_empty());

        constexpr auto size = 10;
        std::unordered_set<entity, entity_hasher> entities;
        for(auto i = 0; i < size; ++i) {
            entities.insert(es.push());
        }
        REQUIRE(entities.size() == size);
        REQUIRE(es.size() == size);
    }

    SECTION("Iterator") {
        REQUIRE(es.begin() == es.end());
        std::ignore = es.push();
        REQUIRE((es.begin() + 1) == es.end());
        es.erase(0);
        REQUIRE(es.begin() == es.end());
    }

    SECTION("Each") {
        constexpr auto size = 10;
        std::unordered_set<entity, entity_hasher> entities;
        for(auto i = 0; i < size; ++i) {
            entities.insert(es.push());
        }
        es.each([](ecs_registry &reg, entity en) {
            reg.emplace_component<int>(en, 10);
        });

        for(const auto &en: es) {
            REQUIRE(reg.has_components<int>(en));
        }
    }
}

struct listener {
    void on_other_entity_update(entity en) {
        this->en = en;
    }
    entity en;
};

TEST_CASE("Signal emission") {
    ecs_registry reg;
    entities_holder es{reg};

    SECTION("On create") {
        listener lis{};

        es.on_create().connect<&listener::on_other_entity_update>(lis);
        const auto created_en = es.push();
        REQUIRE(lis.en == created_en);

        es.on_create().disconnect<&listener::on_other_entity_update>(lis);
        std::ignore = es.push();
        REQUIRE(lis.en == created_en);
    }

    SECTION("On destroy") {
        listener lis{};

        es.on_destroy().connect<&listener::on_other_entity_update>(lis);
        const auto created_en = es.push();
        es.erase(0);
        REQUIRE(lis.en == created_en);

        es.on_destroy().disconnect<&listener::on_other_entity_update>(lis);
        std::ignore = es.push();
        es.erase(0);
        REQUIRE(lis.en == created_en);
    }
}
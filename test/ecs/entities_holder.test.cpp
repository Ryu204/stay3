#include <tuple>
#include <unordered_set>
#include <catch2/catch_all.hpp>

using Catch::Matchers::Contains;
import stay3.ecs;
using namespace st;

TEST_CASE("Entity manipulation") {
    ecs_registry reg;
    entities_holder es{reg};

    SECTION("Push and erase") {
        const auto en = es.create();
        REQUIRE(reg.contains_entity(en));

        const auto en2 = es.create();
        es.destroy(1);
        REQUIRE_FALSE(reg.contains_entity(en2));
        REQUIRE(reg.contains_entity(en));

        es.destroy(0);
        REQUIRE_FALSE(reg.contains_entity(en));
    }

    SECTION("Size and empty check") {
        REQUIRE(es.is_empty());

        constexpr auto size = 10;
        std::unordered_set<entity, entity_hasher> entities;
        for(auto i = 0; i < size; ++i) {
            entities.insert(es.create());
        }
        REQUIRE(entities.size() == size);
        REQUIRE(es.size() == size);
    }

    SECTION("Iterator") {
        REQUIRE(es.begin() == es.end());
        std::ignore = es.create();
        REQUIRE((es.begin() + 1) == es.end());
        es.destroy(0);
        REQUIRE(es.begin() == es.end());
    }

    SECTION("Each") {
        constexpr auto size = 10;
        std::unordered_set<entity, entity_hasher> entities;
        for(auto i = 0; i < size; ++i) {
            entities.insert(es.create());
        }
        es.each([](ecs_registry &reg, entity en) {
            reg.add_component<int>(en, 10);
        });

        for(const auto &en: es) {
            REQUIRE(reg.has_components<int>(en));
        }
    }

    SECTION("Clear") {
        constexpr auto size = 5;
        for(auto i = 0; i < size; ++i) {
            es.create();
        }
        REQUIRE(es.size() == size);

        es.clear();
        REQUIRE(es.is_empty());
        REQUIRE(es.begin() == es.end());
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
        const auto created_en = es.create();
        REQUIRE(lis.en == created_en);

        es.on_create().disconnect<&listener::on_other_entity_update>(lis);
        std::ignore = es.create();
        REQUIRE(lis.en == created_en);
    }

    SECTION("On destroy") {
        listener lis{};

        es.on_destroy().connect<&listener::on_other_entity_update>(lis);
        const auto created_en = es.create();
        es.destroy(0);
        REQUIRE(lis.en == created_en);

        es.on_destroy().disconnect<&listener::on_other_entity_update>(lis);
        std::ignore = es.create();
        es.destroy(0);
        REQUIRE(lis.en == created_en);

        SECTION("On destructor") {
            std::vector<entity> entities;
            {
                entities_holder es2{reg};
                entities = {es2.create(), es2.create()};
                es2.on_destroy().connect<&listener::on_other_entity_update>(lis);
            }
            for(auto en: entities) {
                REQUIRE_FALSE(reg.contains_entity(en));
            }
            REQUIRE_THAT(entities, Contains(lis.en));
        }
    }
}

TEST_CASE("Iteration") {
    ecs_registry reg;
    entities_holder es{reg};

    SECTION("Ranged-for iteration") {
        std::unordered_set<entity, entity_hasher> entities;
        constexpr auto size = 5;
        for(auto i = 0; i < size; ++i) {
            entities.insert(es.create());
        }
        std::unordered_set<entity, entity_hasher> iterated;
        for(auto &en: es) {
            iterated.insert(en);
        }
        REQUIRE(iterated == entities);

        iterated.clear();
        const entities_holder &const_es = es;
        for(const auto &en: const_es) {
            iterated.insert(en);
        }
        REQUIRE(iterated == entities);
    }
}

#include <iterator>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <catch2/catch_all.hpp>

using Catch::Matchers::Contains;
import stay3.ecs;
using namespace st;

struct fixture {
    /**
     * @brief Because `entities_holder` requires manual call to `discard`, we must keep a mapping from entity to holder
     */

    struct holder_wrapper {
        fixture *fi;
        entities_holder *holder;
        holder_wrapper(fixture &fi, entities_holder &holder)
            : fi{&fi}, holder{&holder} {
            holder.on_create().connect<[](holder_wrapper &this_wrapper, entity en) {
                this_wrapper.fi->entity_to_holder.emplace(en, &this_wrapper);
            }>(*this);
        }
    };
    ecs_registry reg;
    std::unordered_map<entity, holder_wrapper *, entity_hasher, entity_equal> entity_to_holder;
    std::vector<holder_wrapper> holders;

    fixture() {
        reg.on_entity_destroyed().connect<[](fixture &fi, entity en) {
            fi.entity_to_holder.at(en)->holder->discard(en);
        }>(*this);
    }

    void init_holder(entities_holder &holder) {
        holders.emplace_back(*this, holder);
    }
};

TEST_CASE("Type trait") {
    STATIC_REQUIRE(std::input_iterator<entities_holder::const_iterator>);
    STATIC_REQUIRE(std::ranges::range<entities_holder>);
}

TEST_CASE("Entity manipulation") {
    fixture fi;
    entities_holder es{fi.reg};
    fi.init_holder(es);

    SECTION("Push and erase") {
        const auto en = es.create();
        REQUIRE(fi.reg.contains(en));

        const auto en2 = es.create();
        es.destroy(1);
        REQUIRE_FALSE(fi.reg.contains(en2));
        REQUIRE(fi.reg.contains(en));

        es.destroy(0);
        REQUIRE_FALSE(fi.reg.contains(en));

        auto en3 = es.create();
        auto en4 = es.create();
        es.destroy(en4);
        REQUIRE_FALSE(fi.reg.contains(en4));
        REQUIRE(fi.reg.contains(en3));
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
            reg.emplace<int>(en, 10);
        });

        for(const auto &en: es) {
            REQUIRE(fi.reg.contains<int>(en));
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
    fixture fi;
    entities_holder es{fi.reg};
    fi.init_holder(es);

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
                entities_holder es2{fi.reg};
                fi.init_holder(es2);
                entities = {es2.create(), es2.create()};
                es2.on_destroy().connect<&listener::on_other_entity_update>(lis);
            }
            for(auto en: entities) {
                REQUIRE_FALSE(fi.reg.contains(en));
            }
            REQUIRE_THAT(entities, Contains(lis.en));
        }
    }
}

TEST_CASE("Iteration") {
    fixture fi;
    entities_holder es{fi.reg};
    fi.init_holder(es);

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

#include <stdexcept>
#include <catch2/catch_all.hpp>

import stay3;
import stay3.test_helper;

using namespace st;
using Catch::Matchers::Message;

struct entities {
    entity depth1;
    entity depth2;
    entity depth31;
    entity depth32;
};

entities setup(tree_context &ctx) {
    auto &reg = ctx.ecs();
    auto &root = ctx.root();

    auto &depth1_node = root.add_child();
    auto depth1 = depth1_node.entities().create();
    reg.emplace<transform>(depth1);

    auto &depth2_node = depth1_node.add_child();
    auto depth2 = depth2_node.entities().create();
    reg.emplace<transform>(depth2);

    auto &depth31_node = depth2_node.add_child();
    auto depth31 = depth31_node.entities().create();
    reg.emplace<transform>(depth31);

    auto &depth32_node = depth2_node.add_child();
    auto depth32 = depth32_node.entities().create();
    reg.emplace<transform>(depth32);

    return {.depth1 = depth1, .depth2 = depth2, .depth31 = depth31, .depth32 = depth32};
}

void check_transitive_update(tree_context &ctx, const entities &es) {
    sync_global_transform(ctx);
    ctx.ecs().get<mut<transform>>(es.depth2)->scale(2.F);
    sync_global_transform(ctx);
    auto depth3_tf = ctx.ecs().get<global_transform>(es.depth31);
    REQUIRE(approx_equal(depth3_tf->get().scale(), vec3f{2.F}));
}

void check_reparent_update(tree_context &ctx, const entities &es) {
    ctx.ecs().get<mut<transform>>(es.depth2)->scale(3.F);
    sync_global_transform(ctx);

    ctx.get_node(es.depth31).reparent(ctx.get_node(es.depth1));
    sync_global_transform(ctx);

    auto depth3_tf = ctx.ecs().get<global_transform>(es.depth31);
    REQUIRE(approx_equal(depth3_tf->get().scale(), vec3f{1.F}));
}

void check_parent_entity_removed_update(tree_context &ctx, const entities &es) {
    ctx.ecs().get<mut<transform>>(es.depth2)->scale(3.F);
    sync_global_transform(ctx);

    ctx.get_node(es.depth2).entities().clear();
    sync_global_transform(ctx);

    auto depth3_tf = ctx.ecs().get<global_transform>(es.depth31);
    REQUIRE(approx_equal(depth3_tf->get().scale(), vec3f{1.F}));
}

void check_parent_entity_added_update(tree_context &ctx, entities es) {
    ctx.ecs().get<mut<transform>>(es.depth2)->scale(3.F);
    auto &node2 = ctx.get_node(es.depth2);
    node2.entities().clear();
    sync_global_transform(ctx);
    es.depth2 = node2.entities().create();
    ctx.ecs().emplace<mut<transform>>(es.depth2)->scale(5.F);
    sync_global_transform(ctx);

    auto depth3_tf = ctx.ecs().get<global_transform>(es.depth31);
    REQUIRE(approx_equal(depth3_tf->get().scale(), vec3f{5.F}));
}

void check_parent_transform_removed_update(tree_context &ctx, const entities &es) {
    ctx.ecs().get<mut<transform>>(es.depth2)->scale(3.F);
    sync_global_transform(ctx);

    ctx.ecs().destroy<transform>(es.depth2);
    sync_global_transform(ctx);

    auto depth3_tf = ctx.ecs().get<global_transform>(es.depth31);
    REQUIRE(approx_equal(depth3_tf->get().scale(), vec3f{1.F}));
}

void check_parent_transform_added_update(tree_context &ctx, const entities &es) {
    ctx.ecs().get<mut<transform>>(es.depth2)->scale(3.F);
    ctx.ecs().destroy<transform>(es.depth2);
    sync_global_transform(ctx);
    ctx.ecs().emplace<mut<transform>>(es.depth2)->scale(5.F);
    sync_global_transform(ctx);

    auto depth3_tf = ctx.ecs().get<global_transform>(es.depth31);
    REQUIRE(approx_equal(depth3_tf->get().scale(), vec3f{5.F}));
}

#define STAY3_TEST_SYSTEM(function) \
    struct sys_##function { \
        void start(tree_context &ctx) { \
            ens = setup(ctx); \
        } \
        sys_run_result update(seconds, tree_context &ctx) const { \
            function(ctx, ens); \
            return sys_run_result::exit; \
        } \
        entities ens; \
    }; \
    TEST_CASE(#function) { \
        app my_app{{.web = {.exit_main = false}, .use_default_systems = false}}; \
        my_app.systems().add<transform_sync_system>().run_as<sys_type::start>(sys_priority::very_high); \
        REQUIRE_NOTHROW(my_app.systems().add<sys_##function>().run_as<sys_type::start>().run_as<sys_type::update>()); \
        my_app.run(); \
    }

STAY3_TEST_SYSTEM(check_transitive_update);
STAY3_TEST_SYSTEM(check_reparent_update);
STAY3_TEST_SYSTEM(check_parent_entity_added_update);
STAY3_TEST_SYSTEM(check_parent_entity_removed_update);
STAY3_TEST_SYSTEM(check_parent_transform_added_update);
STAY3_TEST_SYSTEM(check_parent_transform_removed_update);
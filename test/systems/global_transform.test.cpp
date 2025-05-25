#include <numbers>

#include <catch2/catch_all.hpp>
import stay3;
import stay3.test_helper;

using namespace st;

namespace {
struct entities {
    entity root_child;
    entity stray;
    entity sibling1;
    entity sibling2;
};

entities set_up_entities(tree_context &ctx) {
    auto &reg = ctx.ecs();

    auto &root = ctx.root();
    auto &root_child = root.add_child();

    auto root_en = root.entities().create();
    reg.emplace<mut<transform>>(root_en)
        ->scale(2.F)
        .rotate(vec_up + vec_right, PI / 4.F)
        .translate(2.F * vec_left);

    auto root_child_en = root_child.entities().create();
    reg.emplace<mut<transform>>(root_child_en)
        ->scale(3.F)
        .rotate(vec_down, PI);

    auto &stray_parent = root.add_child();
    auto &stray = stray_parent.add_child();
    auto stray_en = stray.entities().create();
    reg.emplace<mut<transform>>(stray_en)
        ->scale(0.5F)
        .rotate(vec_down + vec_left + vec_forward, PI * 3.5F)
        .translate(vec_left + vec_back * 2.F);

    auto &sibling1 = stray.add_child();
    auto &sibling2 = stray.add_child();
    auto sib1_en = sibling1.entities().create();
    auto sib2_en = sibling2.entities().create();
    reg.emplace<mut<transform>>(sib1_en)->translate(7.F * vec_forward);
    reg.emplace<mut<transform>>(sib2_en)->translate(vec_down / 2.F);

    return {
        .root_child = root_child_en,
        .stray = stray_en,
        .sibling1 = sib1_en,
        .sibling2 = sib2_en,
    };
}

void check_single_transform(const std::string_view &node_name, tree_context &ctx, entity id_to_check, const vec3f &target_pos, const quaternionf target_orientation, const vec3f &target_scale) {
    INFO(node_name);

    auto &reg = ctx.ecs();
    REQUIRE(reg.contains<global_transform>(id_to_check));
    const auto &global_tf = reg.get<global_transform>(id_to_check)->get();
    {
        INFO("position");
        REQUIRE(approx_equal(global_tf.position(), target_pos, 1e-4F));
    }
    {
        INFO("Scale");
        REQUIRE(approx_equal(global_tf.scale(), target_scale, 1e-4F));
    }
    {
        INFO("Orientation");
        REQUIRE(approx_equal(global_tf.orientation(), target_orientation, 1e-4F));
    }
} // namespace

void check_transforms(tree_context &ctx, const entities &ids) {
    constexpr auto sq3 = std::numbers::inv_sqrt3_v<float>;

    // All these values are found by mocking the scene in Blender
    check_single_transform("root child", ctx, ids.root_child, vec3f{-2.F, 0.F, 0.F}, quaternionf{{0.F, 0.959683F, 0.281084F}, 211.4F / 180.F * PI}, vec3f{6.F});
    check_single_transform("stray", ctx, ids.stray, vec3f{-1.F, 0.F, -2.F}, quaternionf{{-sq3, -sq3, sq3}, 630.F / 180.F * PI}, vec3f{0.5F});
    check_single_transform("sibling 1", ctx, ids.sibling1, vec3f{-0.14594099F, -3.187395F, -0.8333333F}, quaternionf{vec3f{sq3, sq3, -sq3}, PI / 2.F}, vec3f{0.5F});
    check_single_transform("sibling 2", ctx, ids.sibling2, vec3f{-0. - 1.22767F, -0.083333F, -2.061F}, quaternionf{vec3f{sq3, sq3, -sq3}, PI / 2.F}, vec3f{0.5F});
}
} // namespace

struct transform_checker {
    void start(tree_context &ctx) {
        ids = set_up_entities(ctx);
    }

    sys_run_result post_update(seconds, tree_context &ctx) const {
        check_transforms(ctx, ids);
        return sys_run_result::exit;
    }

    entities ids;
};

struct transform_sync_user {
    void start(tree_context &ctx) {
        ids = set_up_entities(ctx);
    }

    sys_run_result update(seconds, tree_context &ctx) const {
        sync_global_transform(ctx);
        check_transforms(ctx, ids);
        return sys_run_result::exit;
    }

    entities ids;
};

struct single_transform_sync_user {
    void start(tree_context &ctx) {
        ids = set_up_entities(ctx);
    }

    sys_run_result update(seconds, tree_context &ctx) const {
        auto &reg = ctx.ecs();
        reg.get<mut<transform>>(ids.stray)->translate(vec_down * 3.F);
        reg.get<mut<transform>>(ids.sibling1)->translate(vec_up * 3.F).rotate(vec_down, PI / 2.F).scale(1.5F);
        sync_global_transform(ctx, ids.sibling1);
        constexpr auto sq3 = std::numbers::inv_sqrt3_v<float>;
        check_single_transform("stray", ctx, ids.stray, vec3f{-1.F, -3.F, -2.F}, quaternionf{{-sq3, -sq3, sq3}, 630.F / 180.F * PI}, vec3f{0.5F});
        check_single_transform("sibling 1", ctx, ids.sibling1, {1.22008F, -5.68739F, -0.467309F}, quaternionf{{0.F, -0.343724F, -0.939071F}, 75.8763F / 180 * PI}, vec3f{0.75F});

        return sys_run_result::exit;
    }

    entities ids;
};

struct transform_setter {
    void start(tree_context &ctx) {
        ids = set_up_entities(ctx);
    }

    sys_run_result update(seconds, tree_context &ctx) const {
        auto &reg = ctx.ecs();
        constexpr auto sq3 = std::numbers::inv_sqrt3_v<float>;
        const transform new_stray_tf{
            vec3f{-1.F, -3.F, -2.F},
            quaternionf{{-sq3, -sq3, sq3}, 630.F / 180 * PI},
            vec3f{0.5F},
        };
        set_global_transform(ctx, ids.stray, new_stray_tf);
        check_single_transform("stray", ctx, ids.stray, vec3f{-1.F, -3.F, -2.F}, quaternionf{{-sq3, -sq3, sq3}, 630.F / 180.F * PI}, vec3f{0.5F});

        reg.get<mut<transform>>(ids.sibling1)->translate(vec_up * 3.F).rotate(vec_down, PI / 2.F).scale(1.5F);
        sync_global_transform(ctx, ids.sibling1);
        check_single_transform("sibling 1", ctx, ids.sibling1, {1.22008F, -5.68739F, -0.467309F}, quaternionf{{0.F, -0.343724F, -0.939071F}, 75.8763F / 180 * PI}, vec3f{0.75F});

        return sys_run_result::exit;
    }

    entities ids;
};

TEST_CASE("Happy path") {
    app my_app{{.assets_dir = "../assets", .use_default_systems = false}};
    my_app.systems()
        .add<transform_sync_system>()
        .run_as<sys_type::start>(sys_priority::very_high)
        .run_as<sys_type::post_update>(sys_priority::high);

    SECTION("Sync system changes global transform correctly") {
        my_app.systems()
            .add<transform_checker>()
            .run_as<sys_type::start>()
            .run_as<sys_type::post_update>();

        my_app.run();
    }

    SECTION("Sync free function changes global transform correctly") {
        my_app.systems()
            .add<transform_sync_user>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();

        my_app.run();
    }

    SECTION("Single free function changes global transform correctly") {
        my_app.systems()
            .add<single_transform_sync_user>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();

        my_app.run();
    }

    SECTION("Single setter function changes global transform correctly") {
        my_app.systems()
            .add<transform_setter>()
            .run_as<sys_type::start>()
            .run_as<sys_type::update>();

        my_app.run();
    }
}
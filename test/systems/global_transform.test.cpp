#include <format>
#include <numbers>
#include <stdexcept>

#include <catch2/catch_all.hpp>
import stay3;
import stay3.test_helper;

using namespace st;
using Catch::Matchers::Message;

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
    auto root_tf = reg.add_component<transform>(root_en);
    root_tf->scale(2.F).rotate(vec_up + vec_right, PI / 4.F).translate(2.F * vec_left);

    auto root_child_en = root_child.entities().create();
    auto child_tf = reg.add_component<transform>(root_child_en);
    child_tf->scale(3.F).rotate(vec_down, PI);

    auto &stray_parent = root.add_child();
    auto &stray = stray_parent.add_child();
    auto stray_en = stray.entities().create();
    auto stray_tf = reg.add_component<transform>(stray_en);
    stray_tf->scale(0.5F).rotate(vec_down + vec_left + vec_forward, PI * 3.5F).translate(vec_left + vec_back * 2.F);

    auto &sibling1 = stray.add_child();
    auto &sibling2 = stray.add_child();
    auto sib1_en = sibling1.entities().create();
    auto sib2_en = sibling2.entities().create();
    auto sib1_tf = reg.add_component<transform>(sib1_en);
    auto sib2_tf = reg.add_component<transform>(sib2_en);
    sib1_tf->translate(7.F * vec_forward);
    sib2_tf->translate(vec_down / 2.F);

    return {
        .root_child = root_child_en,
        .stray = stray_en,
        .sibling1 = sib1_en,
        .sibling2 = sib2_en};
}

void check_single_transform(const std::string &test_name, tree_context &ctx, entity id_to_check, const vec3f &target_pos, const quaternionf target_orientation, const vec3f &target_scale) {
    if(!ctx.ecs().has_components<global_transform>(id_to_check)) {
        throw std::runtime_error{"No global component"};
    }
    const auto &global_tf = ctx.ecs().get_component<const global_transform>(id_to_check)->get();

#define THROW_IF_NOT_EQUAL(x, y, z) \
    if(!approx_equal((x), (y), (z))) { \
        throw std::runtime_error{std::format("[{}] {} and {} are different", test_name, #x, #y)}; \
    }

    THROW_IF_NOT_EQUAL(global_tf.position(), target_pos, 1e-4F);
    THROW_IF_NOT_EQUAL(global_tf.scale(), target_scale, 1e-4F);
    THROW_IF_NOT_EQUAL(global_tf.orientation(), target_orientation, 1e-4F);

#undef THROW_IF_NOT_EQUAL
} // namespace

void check_transforms(tree_context &ctx, const entities &ids) {
    constexpr auto sq3 = std::numbers::inv_sqrt3_v<float>;

    // All these values are found by mocking the scene in Blender
    check_single_transform("root child", ctx, ids.root_child, vec3f{-2.F, 0.F, 0.F}, quaternionf{{0.F, 0.959683F, 0.281084F}, 211.4F / 180.F * PI}, vec3f{6.F});

    check_single_transform("stray child", ctx, ids.stray, vec3f{-1.F, 0.F, -2.F}, quaternionf{{-sq3, -sq3, sq3}, 630.F / 180.F * PI}, vec3f{0.5F});

    check_single_transform("sibling 1", ctx, ids.sibling1, vec3f{-0.14594099F, -3.187395F, -0.8333333F}, quaternionf{vec3f{sq3, sq3, -sq3}, PI / 2.F}, vec3f{0.5F});

    check_single_transform("sibling 2", ctx, ids.sibling2, vec3f{-0. - 1.22767F, -0.083333F, -2.061F}, quaternionf{vec3f{sq3, sq3, -sq3}, PI / 2.F}, vec3f{0.5F});
}
} // namespace

struct transform_user {
    void start(tree_context &ctx) {
        ids = set_up_entities(ctx);
    }

    void post_update(seconds, tree_context &ctx) const {
        check_transforms(ctx, ids);
        throw std::runtime_error{"OK"}; // TODO: allow system to request game loop exit
    }

    entities ids;
};

struct transform_sync_user {
    void start(tree_context &ctx) {
        ids = set_up_entities(ctx);
    }

    void update(seconds, tree_context &ctx) const {
        sync_global_transform(ctx);
        check_transforms(ctx, ids);
        throw std::runtime_error{"OK"}; // TODO: allow system to request game loop exit
    }

    entities ids;
};

TEST_CASE("Sync system changes global transform correctly") {
    app my_app;
    my_app.enable_default_systems();
    my_app.systems()
        .add<transform_user>()
        .run_as<sys_type::start>()
        .run_as<sys_type::post_update>();

    REQUIRE_THROWS_MATCHES(my_app.run(), std::runtime_error, Message("OK"));
}

TEST_CASE("Sync free function changes global transform correctly") {
    app my_app;
    my_app.enable_default_systems()
        .systems()
        .add<transform_sync_user>()
        .run_as<sys_type::start>()
        .run_as<sys_type::update>();

    REQUIRE_THROWS_MATCHES(my_app.run(), std::runtime_error, Message("OK"));
}
#include <utility>
#include <catch2/catch_all.hpp>

import stay3.node;
using Catch::Matchers::UnorderedRangeEquals;

struct reparented_handler {
    void on_event(st::tree_context::node_reparented_args args) {
        this->args = args;
    }

    st::tree_context::node_reparented_args args;
};

TEST_CASE("node basic functionality") {
    st::tree_context context;
    auto &root = context.root();

    SECTION("Root node is correctly registered") {
        REQUIRE(root.id() == context.root().id());
        REQUIRE(root.is_root());
        REQUIRE((&context.get_node(root.id())) == (&root));
    }

    SECTION("Adding child nodes") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        REQUIRE(child1.id() != child2.id());
        REQUIRE((&context.get_node(child1.id())) == (&child1));
        SECTION("Child is not root") {
            REQUIRE_FALSE(child1.is_root());
        }
    }

    SECTION("Destroy child nodes") {
        REQUIRE(root.begin() == root.end());
        auto &child1 = root.add_child();
        REQUIRE(root.begin() != root.end());
        root.destroy_child(child1.id());
        REQUIRE(root.begin() == root.end());
        root.add_child();
        root.add_child();
        root.destroy_children();
        REQUIRE(root.begin() == root.end());
    }

    SECTION("Parent-child relationship is maintained") {
        auto &child = root.add_child();
        REQUIRE(&child.parent() == &root);
    }

    SECTION("Reparenting nodes") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        child1.reparent(child2);
        REQUIRE(&child1.parent() == &child2);

        SECTION("Publish signal") {
            reparented_handler handler;
            context.on_node_reparented().connect<&reparented_handler::on_event>(handler);
            child1.reparent(root);
            const auto &args = handler.args;
            REQUIRE(args.current == child1.id());
            REQUIRE(args.old_parent == child2.id());
            REQUIRE(args.new_parent == root.id());
        }
    }

    SECTION("Ancestor relationship") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        auto &child3 = child1.add_child();

        REQUIRE(root.is_ancestor_of(child3));
        REQUIRE(child1.is_ancestor_of(child3));
        REQUIRE_FALSE(child2.is_ancestor_of(child3));
        REQUIRE_FALSE(child3.is_ancestor_of(child3));
    }

    SECTION("Traversal") {
        auto &child1 = root.add_child();
        auto &child2 = root.add_child();
        auto &child3 = child1.add_child();
        auto &child4 = child1.add_child();
        auto &child5 = child2.add_child();

        int current_order = 0;
        std::unordered_map<st::node::id_type, int> visit_order;

        SECTION("Void function") {
            root.traverse([&visit_order, &current_order](auto &&node) {
                visit_order.emplace(node.id(), ++current_order);
            });
#define ORDER(x) (visit_order[(x).id()])
            REQUIRE(ORDER(root) < ORDER(child1));
            REQUIRE(ORDER(root) < ORDER(child2));

            REQUIRE(ORDER(child1) < ORDER(child3));
            REQUIRE(ORDER(child1) < ORDER(child4));

            REQUIRE(ORDER(child2) < ORDER(child5));
        }

        SECTION("Return result function") {
            std::unordered_map<st::node::id_type, st::node::id_type> sum_up_to_root;
            auto calc = [&visit_order, &sum_up_to_root, &current_order](auto &node, auto cur_sum) {
                visit_order.emplace(node.id(), ++current_order);
                cur_sum += node.id();
                sum_up_to_root.emplace(node.id(), cur_sum);
                return cur_sum;
            };
            root.traverse(std::move(calc), 0);
            SECTION("Order") {
                REQUIRE(ORDER(root) < ORDER(child1));
                REQUIRE(ORDER(root) < ORDER(child2));

                REQUIRE(ORDER(child1) < ORDER(child3));
                REQUIRE(ORDER(child1) < ORDER(child4));

                REQUIRE(ORDER(child2) < ORDER(child5));
            }
            SECTION("Value") {
                REQUIRE(sum_up_to_root[root.id()] == root.id());
                REQUIRE(sum_up_to_root[child1.id()] == (root.id() + child1.id()));
                REQUIRE(sum_up_to_root[child3.id()] == (root.id() + child1.id() + child3.id()));
                REQUIRE(sum_up_to_root[child4.id()] == (root.id() + child1.id() + child4.id()));
                REQUIRE(sum_up_to_root[child2.id()] == (root.id() + child2.id()));
                REQUIRE(sum_up_to_root[child5.id()] == (root.id() + child2.id() + child5.id()));
            }
        }
    }

    SECTION("Iterators") {
        REQUIRE(root.begin() == root.end());
        REQUIRE(std::as_const(root).begin() == std::as_const(root).end());

        std::vector<st::node::id_type> children;
        constexpr auto children_count = 5;
        children.reserve(children_count);
        for(auto i = 0; i < children_count; ++i) {
            children.emplace_back(root.add_child().id());
        }
        decltype(children) checked_children;

        SECTION("Const iter") {
            const auto &croot = root;
            for(const auto &child: croot) {
                checked_children.emplace_back(child.id());
            }
            REQUIRE_THAT(checked_children, UnorderedRangeEquals(children));
        }

        SECTION("Iter") {
            for(auto &child: root) {
                checked_children.emplace_back(child.id());
            }
            REQUIRE_THAT(checked_children, UnorderedRangeEquals(children));
        }
    }
}

TEST_CASE("Destruction is safe and can be called multiple times") {
    st::tree_context ctx;
    auto &deep_node = ctx.root().add_child().add_child().add_child().add_child();
    auto &shallow_node = ctx.root().add_child();

    auto &reg = ctx.ecs();
    reg.emplace<float>(deep_node.entities().create());
    reg.emplace<std::string>(shallow_node.entities().create(), "I am writing test");
    reg.emplace<std::string_view>(deep_node.parent().entities().create(), "string_view");
    deep_node.reparent(shallow_node);

    REQUIRE_NOTHROW(ctx.destroy_tree());
    REQUIRE_NOTHROW(ctx.destroy_tree());
    REQUIRE_NOTHROW(ctx.destroy_tree());
}

export module stay3.system.transform;

import stay3.core;
import stay3.node;
import stay3.ecs;

export namespace st {
/**
 * @brief Global transform of an entity `E` in node `N`.
 *
 * It is computed from `E`'s local transform and global transform
 * of first entity in `N`'s parent if any, or else origin
 */
class global_transform {
public:
    [[nodiscard]] const transform &get() const;

private:
    friend void sync_global_transform(tree_context &);
    friend const global_transform &sync_global_transform(tree_context &, entity);
    friend void set_global_transform(tree_context &, entity, const transform &);
    transform global;
};

class transform_sync_system {
public:
    static void start(tree_context &ctx);
    static void post_update(seconds, tree_context &ctx);
};

/**
 * @brief Sync all `global_transform` based on local `transform`
 */
void sync_global_transform(tree_context &ctx);

/**
 * @brief Sync single `global_transform`
 */
const global_transform &sync_global_transform(tree_context &ctx, entity en);

void set_global_transform(tree_context &ctx, entity en, const transform &value);

} // namespace st
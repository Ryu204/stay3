module;

#include <cassert>

export module stay3.system.physics;

import stay3.node;
import stay3.core;
import stay3.physics;

namespace st {

export class physics_system {
public:
    physics_system(const physics_config &settings = {})
        : m_world{settings} {}
    // Currently system wrapper impl uses `std::any` which requires copy ctor for no real benefit
    physics_system(const physics_system &) {
        assert(false && "Cannot copy");
    }
    physics_system(physics_system &&) noexcept = delete;
    physics_system &operator=(const physics_system &) = delete;
    physics_system &operator=(physics_system &&) noexcept = delete;
    ~physics_system() = default;

    static void start(tree_context &ctx) {
        setup_signals(ctx);
    }

    void update(seconds delta, tree_context &) {
        m_world.update(delta);
    }

private:
    static void setup_signals(tree_context &ctx) {
    }

    physics_world m_world;
};
} // namespace st
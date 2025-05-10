module;

export module stay3.graphics.core:material;

import stay3.ecs;
import :texture;

export namespace st {

struct material {
    component_ref<texture_2d> texture;
};

} // namespace st
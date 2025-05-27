module;

export module stay3.graphics.core:rendered_mesh;

import stay3.ecs;
import :vertex;
import :material;

export namespace st {
/**
 * @brief This component represent single visible object in the scene
 */
struct rendered_mesh {
    component_ref<mesh_data> mesh;
    component_ref<material> mat;
};
} // namespace st
module;

export module stay3.graphics:camera;

import stay3.core;

export namespace st {

struct camera {
    static constexpr auto default_fov{70 * PI / 180};
    static constexpr auto default_near{0.01F};
    static constexpr auto default_far{100.F};
    static constexpr auto default_ratio{1.F};
    static constexpr vec4f default_background_color{0.F, 0.F, 0.F, 1.F};

    float fov{default_fov};
    float ratio{default_ratio};
    float near{default_near};
    float far{default_far};
    vec4f clear_color{default_background_color};
};

struct main_camera {};

} // namespace st
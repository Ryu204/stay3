module;

#include <cstdint>
#include <optional>
#include <variant>

export module stay3.graphics:camera;

import stay3.core;

export namespace st {

enum class camera_type : std::uint8_t {
    perspective,
    orthographic,
};
struct camera {
    static constexpr auto default_near{0.01F};
    static constexpr auto default_far{100.F};
    static constexpr vec4f default_background_color{0.F, 0.F, 0.F, 1.F};

    struct perspective_data {
        static constexpr auto default_fov{70 * PI / 180};
        float fov{default_fov};
    };
    struct orthographic_data {
        static constexpr auto default_width{10.F};
        float width{default_width};
    };

    std::optional<float> ratio{std::nullopt};
    float near{default_near};
    float far{default_far};
    vec4f clear_color{default_background_color};
    std::variant<perspective_data, orthographic_data> data{perspective_data{}};

    [[nodiscard]] camera_type type() const {
        return std::visit(
            visit_helper{
                [](const perspective_data &) { return camera_type::perspective; },
                [](const orthographic_data &) { return camera_type::orthographic; },
            },
            data);
    }
    [[nodiscard]] const perspective_data &perspective() const {
        return std::get<0>(data);
    }
    perspective_data &perspective() {
        return std::get<0>(data);
    }
    [[nodiscard]] const orthographic_data &orthographic() const {
        return std::get<1>(data);
    }
    orthographic_data &orthographic() {
        return std::get<1>(data);
    }
};

struct main_camera {};

} // namespace st
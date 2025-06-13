module;

#include <cstdint>

export module stay3.physics:rigidbody;

namespace st {
export struct rigidbody {
public:
    enum class type : std::uint8_t {
        dynamic,
        fixed,
        kinematic,
    };
    static constexpr auto default_friction = 0.2F;
    static constexpr auto default_linear_damping = 0.05F;
    static constexpr auto default_angular_damping = 0.05F;

    type motion_type{type::dynamic};
    bool is_sensor{false};
    bool allow_sleep{true};
    float friction{default_friction};
    float linear_damping{default_linear_damping};
    float angular_damping{default_angular_damping};
};
} // namespace st
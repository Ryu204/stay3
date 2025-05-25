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
    type motion_type{type::dynamic};
    bool is_sensor{false};
    bool allow_sleep{true};
};
} // namespace st
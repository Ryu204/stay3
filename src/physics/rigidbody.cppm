module;

#include <cstdint>

export module stay3.physics:rigidbody;

namespace st {
export enum class rigidbody: std::uint8_t {
    dynamic,
    fixed,
    kinematic,
};
} // namespace st
module;

#include <numbers>

export module stay3.core:math;

export namespace st {

inline constexpr auto PI = std::numbers::pi_v<float>;
inline constexpr float EPS = 1e-6F;
using radians = float;

} // namespace st
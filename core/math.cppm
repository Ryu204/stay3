module;

#include <numbers>

export module stay3.core:math;

export namespace st {

constexpr auto PI = std::numbers::pi_v<float>;
constexpr float EPS = 1e-6F;
using radians = float;

} // namespace st
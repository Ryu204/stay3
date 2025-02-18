module;

#include <numbers>

export module stay3.system:math;

export namespace st {

inline constexpr auto PI = std::numbers::pi_v<float>;
using radians = float;

} // namespace st
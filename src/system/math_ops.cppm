module;

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
export module stay3.system:math_ops;

export namespace st {
using glm::operator+;
using glm::operator-;
using glm::operator*;
using glm::operator/;
using glm::operator==;
using glm::operator!=;
} // namespace st
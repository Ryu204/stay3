module;

#include <cmath>
#include <iostream>
export module stay3.test_helper;

import stay3;

export namespace st {

std::ostream &operator<<(std::ostream &os, const mat4f &mat) {
    os << "mat4f[\n";
    for(int i = 0; i < 4; ++i) {
        os << "  ";
        for(int j = 0; j < 4; ++j) {
            os << mat[i][j];
            if(j < 3) { os << ", "; }
        }
        os << "\n";
    }
    os << "]";
    return os;
}

bool approx_equal(const vec3f &first, const vec3f &second, float epsilon = 1e-6F) {
    return (std::abs(first.x - second.x) < epsilon) && (std::abs(first.y - second.y) < epsilon) && (std::abs(first.z - second.z) < epsilon);
}

bool approx_equal(const quaternionf &first, const quaternionf &second, float epsilon = 1e-6F) {
    return (std::abs(first.angle() - second.angle()) < epsilon) && approx_equal(first.axis(), second.axis(), epsilon);
}

bool approx_equal(const mat4f &first, const mat4f &second, float epsilon = 1e-4f) {
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            if(std::abs(first[i][j] - second[i][j]) > epsilon) {
                std::cout << first << '\n'
                          << second;
                return false;
            }
        }
    }
    return true;
}

} // namespace st
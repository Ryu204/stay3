module;

#include <cmath>
#include <format>
#include <iostream>

#ifdef __EMSCRIPTEN__
#    include <catch2/catch_all.hpp>
#    include <emscripten.h>
#endif

export module stay3.test_helper;

import stay3;

export namespace st {

#ifdef __EMSCRIPTEN__
/** @brief In case wasm runtime does not exit and report back to JavaScript, the compiled C++ code will do it */
struct wasm_runtime_may_not_exit: public Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;
    void testRunEnded(Catch::TestRunStats const &test_run_stats) override {
        if(test_run_stats.aborting) {
            EM_ASM(window.testExitStatus = -1);
        } else {
            EM_ASM(window.testExitStatus = 0);
        }
    }
};
#endif

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

std::ostream &operator<<(std::ostream &os, const vec3f &vec) {
    os << "vec3f(";
    for(int i = 0; i < 3; ++i) {
        os << vec[i];
        if(i < 2) { os << ","; };
    }
    os << ")";
    return os;
}

std::ostream &operator<<(std::ostream &os, const quaternionf &quat) {
    os << std::format("quaternionf({},{},{},{})", quat.x, quat.y, quat.z, quat.w);
    return os;
}

bool approx_equal(const vec3f &first, const vec3f &second, float epsilon = 1e-6F) {
    if(std::abs(first.x - second.x) < epsilon && std::abs(first.y - second.y) < epsilon && std::abs(first.z - second.z) < epsilon) {
        return true;
    };
    std::cout << first << "\n"
              << second << "\n";
    return false;
}

bool approx_equal(const quaternionf &first, const quaternionf &second, float epsilon = 1e-6F, bool print = true) {
    if((std::abs(first.w - second.w) <= epsilon && std::abs(first.x - second.x) <= epsilon && std::abs(first.y - second.y) <= epsilon && std::abs(first.z - second.z) <= epsilon)) {
        return true;
    }
    if(print) {
        std::cout << first << "\n"
                  << second << "\n";
    }
    return false;
}

bool approx_equal_orientation(const quaternionf &first, const quaternionf &second, float epsilon = 1e-6F, bool print = true) {
    if((std::abs(first.w - second.w) <= epsilon && std::abs(first.x - second.x) <= epsilon && std::abs(first.y - second.y) <= epsilon && std::abs(first.z - second.z) <= epsilon)) {
        return true;
    }
    if((std::abs(first.w + second.w) <= epsilon && std::abs(first.x + second.x) <= epsilon && std::abs(first.y + second.y) <= epsilon && std::abs(first.z + second.z) <= epsilon)) {
        return true;
    }
    if(print) {
        std::cout << first << "\n"
                  << second << "\n";
    }
    return false;
}

bool same_orientation(const quaternionf &first, const quaternionf &second, float epsilon = 1e-6F) {
    const auto minus_second = quaternionf::wxyz(-second.w, -second.x, -second.y, -second.z);
    if(approx_equal(first, second, epsilon, false) || approx_equal(first, minus_second, epsilon, false)) {
        return true;
    }
    std::cout << first << "\n"
              << second << "\n";
    return false;
}

bool approx_equal(const mat4f &first, const mat4f &second, float epsilon = 1e-4f) {
    for(int i = 0; i < 4; ++i) {
        for(int j = 0; j < 4; ++j) {
            if(std::abs(first[i][j] - second[i][j]) > epsilon) {
                std::cout << first << "\n"
                          << second << "\n";
                return false;
            }
        }
    }
    return true;
}

} // namespace st
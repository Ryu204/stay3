module;

#include <random>

export module stay3.core:color;

import :vector;

export namespace st {
vec4f random_color() {
    thread_local std::mt19937_64 gen{std::random_device{}()};
    thread_local std::uniform_real_distribution<float> dist{0.F, 1.F};
    return vec4f{dist(gen), dist(gen), dist(gen), 1.F};
}
} // namespace st
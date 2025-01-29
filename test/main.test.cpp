#include <cassert>

import stay3;

int main() {
    constexpr auto first = 1;
    constexpr auto second = 5;
    constexpr auto result = 6;

    static_assert(st::add(first, second) == result);

    return 0;
}
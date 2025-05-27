module;

#include <cstdint>
#include <string_view>
#include <GLFW/glfw3.h>

export module stay3.input:keyboard;

export namespace st {

enum class scancode : std::uint16_t {
    a = GLFW_KEY_A,
    s = GLFW_KEY_S,
    d = GLFW_KEY_D,
    w = GLFW_KEY_W,
    q = GLFW_KEY_Q,
    e = GLFW_KEY_E,

    num_1 = GLFW_KEY_1,
    num_2 = GLFW_KEY_2,
    num_3 = GLFW_KEY_3,

    up = GLFW_KEY_UP,
    down = GLFW_KEY_DOWN,
    left = GLFW_KEY_LEFT,
    right = GLFW_KEY_RIGHT,

    enter = GLFW_KEY_ENTER,
    space = GLFW_KEY_SPACE,
    esc = GLFW_KEY_ESCAPE,
    tab = GLFW_KEY_TAB,
};

enum class key_status : std::uint8_t {
    pressed,
    released,
};

std::string_view get_name(scancode code) {
    switch(code) {
#define NAME_AS_STR(key) \
    case scancode::key: \
        return #key;

        NAME_AS_STR(a)
        NAME_AS_STR(s)
        NAME_AS_STR(d)
        NAME_AS_STR(w)
        NAME_AS_STR(q)
        NAME_AS_STR(e)
        NAME_AS_STR(up)
        NAME_AS_STR(down)
        NAME_AS_STR(left)
        NAME_AS_STR(right)
        NAME_AS_STR(enter)
        NAME_AS_STR(space)
        NAME_AS_STR(esc)
        NAME_AS_STR(tab)

#undef NAME_AS_STR
#define NAME_OF_CODE(code, name) \
    case scancode::code: \
        return #name;

        NAME_OF_CODE(num_1, 1)
        NAME_OF_CODE(num_2, 2)
        NAME_OF_CODE(num_3, 3)

#undef NAME_OF_CODE

    default:
        return "undefined";
    }
}
} // namespace st
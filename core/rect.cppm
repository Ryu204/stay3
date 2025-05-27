export module stay3.core:rect;

import :vector;

export namespace st {
template<typename type>
struct rect {
    /**
     * @brief Top-left coordinate
     */
    vec2<type> position;
    vec2<type> size;

    type bottom() const {
        return position.y + size.y;
    }
    type top() const {
        return position.y;
    }
    type left() const {
        return position.x;
    }
    type right() const {
        return position.x + size.x;
    }
};

using rectf = rect<float>;
} // namespace st
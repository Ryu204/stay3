module;

#include <string_view>

export module stay3.system:error;

export namespace st {
struct error {
    using message_type = std::string_view;

    error(const std::string_view &message)
        : message{message} {}

    message_type message;
};
} // namespace st
module;

#include <exception>
#include <string>

export module stay3.system:error;

export namespace st {
struct error: public std::exception {
    using message_type = std::string;

    error(std::string message)
        : message{std::move(message)} {}

    [[nodiscard]] const char *what() const noexcept override {
        return message.c_str();
    }

    message_type message;
};
} // namespace st
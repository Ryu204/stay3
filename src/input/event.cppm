module;

#include <type_traits>
#include <utility>
#include <variant>

export module stay3.input:event;

namespace st {

template<typename ev, typename all>
struct is_variant_member {
    static constexpr auto value = false;
};
template<typename ev, typename... evs>
struct is_variant_member<ev, std::variant<evs...>>
    : public std::disjunction<std::is_same<ev, evs>...> {};

export class event {
public:
    struct none {};

    struct close_requested {};

    struct event_b {
        int foo{};
    };

    using data_type = std::variant<none, close_requested, event_b>;
    template<typename t>
    constexpr static bool is_event_type = is_variant_member<t, data_type>::value;

    event()
        : m_data{none{}} {}

    template<typename ev>
        requires(is_event_type<ev>)
    event(const ev &val)
        : m_data{val} {}

    template<typename ev>
        requires(is_event_type<ev>)
    [[nodiscard]] bool is() const {
        return std::holds_alternative<ev>(m_data);
    }

    template<typename ev>
        requires(is_event_type<ev>)
    const ev *try_get() const {
        return std::get_if<ev>(&m_data);
    }

    template<typename ev>
    ev *try_get() {
        return const_cast<ev *>(std::as_const(*this).try_get<ev>());
    }

private:
    data_type m_data;
};
} // namespace st
module;

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <angelscript.h>

export module stay3.system.script.angelscript:assertion;

namespace st::ags {

constexpr bool is_alpha_underscore(char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_';
}

constexpr bool is_alpha_num_underscore(char ch) {
    return (ch >= '0' && ch <= '9') || is_alpha_underscore(ch);
}

const std::string whitespaces = " \t\n";

constexpr bool is_whitespace(char ch) {
    return whitespaces.contains(ch);
}

constexpr bool is_valid_assertion_symbol(const std::string &symbol) {
    if(symbol.empty()) { return false; }
    if(!is_alpha_underscore(symbol[0])) { return false; }
    return std::ranges::all_of(symbol, is_alpha_num_underscore);
}

std::uint32_t get_continuous_repeated_times(const std::string &content, std::string::size_type pos) {
    assert(pos >= 0 && pos < content.size() && "Invalid pos");
    const auto ch = content[pos];
    for(auto i = static_cast<int>(pos); i >= 0; --i) {
        if(content[i] != ch) {
            return pos - i;
        }
    }
    return pos + 1;
}

// NOLINTNEXTLINE(*-swappable-parameters)
constexpr bool is_whitespace_or_marked(const std::string &str, char marked_char, std::uint32_t start, std::uint32_t end_exclusive) {
    for(auto i = start; i < end_exclusive; ++i) {
        if(!is_whitespace(str[i]) && str[i] != marked_char) { return false; }
    }
    return true;
}

struct mark_comments_and_string_result {
    bool is_ok{false};
    std::optional<std::unordered_set<std::uint32_t>> result{std::nullopt};
    std::optional<std::string> error_message{std::nullopt};
};

// TODO: I am implementing debug/release assert keyword in script
// in release mode assert will be stripped with these rule:
// [; or whitespace or start of file][assert keyword]+[whitespaces]+(+[matching)]+[whitespaces]+;
// becomes
// ;
// We need to test this file thoroughly, so it saves us big time in the future
// Then, we add option to turn on/off assertion in script,
// And then implement tests in script
mark_comments_and_string_result mark_comments_and_string(const std::string &content) {
    enum class state : std::uint8_t {
        normal,
        line_comment,
        block_comment,
        char_literal,
        string_literal,
        raw_string_literal,
    };
    static const std::string constants_block_start = "/*";
    static const std::string constants_block_end = "*/";
    static const std::string constants_line_comment = "//";
    constexpr auto constants_eol = '\n';
    static const std::string constants_raw_str_start = "R\"";
    constexpr auto constants_raw_str_open = '(';
    constexpr auto constants_raw_str_close = ')';
    constexpr auto constants_raw_str_end = '"';
    constexpr auto constants_char = '\'';
    constexpr auto constants_str = '"';
    constexpr auto constants_escape_char = '\\';

    std::unordered_set<std::uint32_t> result;
    result.reserve(content.size());

    state current_state = state::normal;
    const auto len = content.size();
    int index = 0;
    std::string raw_str_delimiter;
    while(index < len) {
        const auto ch = content[index];
        switch(current_state) {
        case state::normal: {
            const auto is_start_of_line_comment =
                content.compare(index, constants_line_comment.size(), constants_line_comment) == 0;
            if(is_start_of_line_comment) {
                for(auto i = static_cast<int>(index + constants_line_comment.size() - 1); i >= index; --i) {
                    result.insert(i);
                }
                index += static_cast<int>(constants_line_comment.size());
                current_state = state::line_comment;
                break;
            }
            const auto is_start_of_block_comment =
                content.compare(index, constants_block_start.size(), constants_block_start) == 0;
            if(is_start_of_block_comment) {
                for(auto i = static_cast<int>(index + constants_block_start.size() - 1); i >= index; --i) {
                    result.insert(i);
                }
                index += static_cast<int>(constants_block_start.size());
                current_state = state::block_comment;
                break;
            }
            const auto is_start_of_char = ch == constants_char;
            if(is_start_of_char) {
                result.insert(index);
                ++index;
                current_state = state::char_literal;
                break;
            }
            const auto is_start_of_str = ch == constants_str;
            if(is_start_of_str) {
                result.insert(index);
                ++index;
                current_state = state::string_literal;
                break;
            }
            const auto is_start_of_raw_str =
                content.compare(index, constants_raw_str_start.size(), constants_raw_str_start) == 0;
            if(is_start_of_raw_str) {
                const auto index_of_open = content.find(constants_raw_str_open, index);
                const auto not_found = index_of_open == std::decay_t<decltype(content)>::npos;
                if(not_found) {
                    return {
                        .is_ok = false,
                        .error_message = "Raw string literal is not opened",
                    };
                }
                for(auto i = index; i <= index_of_open; ++i) {
                    result.insert(i);
                }
                raw_str_delimiter = content.substr(
                    index + constants_raw_str_start.size(),
                    index_of_open - index - constants_raw_str_start.size());
                index = static_cast<int>(index_of_open + 1);
                current_state = state::raw_string_literal;
                break;
            }
            ++index;
            break;
        }
        case state::line_comment: {
            const auto is_line_end = ch == constants_eol || index == len - 1;
            if(is_line_end) {
                current_state = state::normal;
                if(ch != constants_eol) {
                    result.insert(index);
                }
            } else {
                result.insert(index);
            }
            ++index;
            break;
        }
        case state::block_comment: {
            result.insert(index);
            const auto is_end_token =
                content.compare(index, constants_block_end.size(), constants_block_end) == 0;
            if(is_end_token) {
                for(auto i = static_cast<int>(index + constants_block_end.size() - 1); i >= index; --i) {
                    result.insert(i);
                }
                index += static_cast<int>(constants_block_end.size());
                current_state = state::normal;
            } else {
                ++index;
            }
            break;
        }
        case state::char_literal: {
            result.insert(index);
            const auto is_char_symbol = ch == constants_char;
            const auto is_prev_escape = index > 0 && content[index - 1] == constants_escape_char
                                        && (get_continuous_repeated_times(content, index - 1) % 2 == 1);
            const auto is_end_token = is_char_symbol && !is_prev_escape;
            if(is_end_token) {
                current_state = state::normal;
            }
            ++index;
            break;
        }
        case state::string_literal: {
            result.insert(index);
            const auto is_str_symbol = ch == constants_str;
            const auto is_prev_escape = index > 0 && content[index - 1] == constants_escape_char
                                        && (get_continuous_repeated_times(content, index - 1) % 2 == 1);
            const auto is_end_token = is_str_symbol && !is_prev_escape;
            if(is_end_token) {
                current_state = state::normal;
            }
            ++index;
            break;
        }
        case state::raw_string_literal: {
            const auto is_close_symbol = ch == constants_raw_str_close;
            const auto is_next_part_delimiter =
                (index + 1 < len)
                && content.compare(index + 1, raw_str_delimiter.size(), raw_str_delimiter) == 0;
            const auto is_next_next_part_end =
                (index + 1 + raw_str_delimiter.size() < len)
                && content[index + 1 + raw_str_delimiter.size()] == constants_raw_str_end;
            const auto is_end_token = is_close_symbol && is_next_part_delimiter && is_next_next_part_end;
            if(is_end_token) {
                current_state = state::normal;
                const auto old_index = index;
                index += static_cast<int>(2 + raw_str_delimiter.size());
                for(auto i = old_index; i < index; ++i) {
                    result.insert(i);
                }
            } else {
                result.insert(index);
                ++index;
            }
            break;
        }
        default:
            assert(false && "Unimplemented");
        }
    }
    switch(current_state) {
    case state::normal:
        return {
            .is_ok = true,
            .result = std::move(result),
        };
    case state::block_comment:
        return {.is_ok = false, .error_message = "Unterminated block comment"};
    case state::line_comment:
        assert(false && "Could not end in line comment state");
        return {.is_ok = false};
    case state::char_literal:
        return {.is_ok = false, .error_message = "Unterminated char literal"};
    case state::raw_string_literal:
        return {.is_ok = false, .error_message = std::format("Unterminated raw string literal, delimiter \"{}\"", raw_str_delimiter)};
    case state::string_literal:
        return {.is_ok = false, .error_message = "Unterminated string literal"};
    default:
        assert(false && "Unimplemented");
        return {.is_ok = false, .error_message = "Unknown error"};
    }
}

export struct strip_assert_statements_params {
    std::string source;
    std::string assertion_symbol{"assert"};
};

export struct strip_assert_statements_result {
    bool is_ok;
    std::optional<std::string> content;
    std::optional<std::string> error_message;
};

export strip_assert_statements_result strip_assert_statements(const strip_assert_statements_params &params) {
    const std::string &source = params.source;
    const std::string &symbol = params.assertion_symbol;
    constexpr auto constants_non_token = '$';
    constexpr auto constants_assert_open = '(';
    constexpr auto constants_assert_close = ')';
    constexpr auto constants_statements_end = ';';
    constexpr auto constants_method_call_char = '.';
    static const std::string constants_method_ptr_call_str = "->";

    if(!is_valid_assertion_symbol(symbol)) {
        return {
            .is_ok = false,
            .error_message = "Invalid assertion symbol",
        };
    }

    const auto marked = mark_comments_and_string(source);
    if(!marked.is_ok) {
        return {
            .is_ok = false,
            .error_message = marked.error_message,
        };
    }
    const auto &marked_indices = marked.result.value();
    auto marked_source = source;
    for(const auto index: marked_indices) {
        marked_source[index] = constants_non_token;
    }

    std::string output;
    output.reserve(source.length());
    int index{};
    const auto len = source.length();
    const auto on_not_assert_start = [&output, &index, &source]() -> void {
        output.push_back(source[index]);
        ++index;
    };
    while(index < len) {
        const auto is_assert_symbol = marked_source.compare(index, symbol.length(), symbol) == 0;
        if(!is_assert_symbol) {
            on_not_assert_start();
            continue;
        }
        const auto is_symbol_suffix_of_something =
            index > 0
            && is_alpha_num_underscore(marked_source[index - 1]);
        if(is_symbol_suffix_of_something) {
            on_not_assert_start();
            continue;
        }
        constexpr auto is_symbol_method_call =
            +[](const std::string &content, int symbol_start) -> bool {
            if(symbol_start <= 0) { return false; }
            static const auto excluded_symbols = whitespaces + constants_non_token;
            const auto prev_meaningful_char_location = content.find_last_not_of(
                excluded_symbols, symbol_start - 1);
            const auto not_found = prev_meaningful_char_location == std::decay_t<decltype(content)>::npos;
            if(not_found) { return false; }
            const auto is_method_call = content[prev_meaningful_char_location] == constants_method_call_char;
            static const auto ptr_call_str_len = static_cast<int>(constants_method_ptr_call_str.size());
            const auto is_method_ptr_call =
                prev_meaningful_char_location >= ptr_call_str_len - 1
                && content.compare(
                       prev_meaningful_char_location - ptr_call_str_len + 1,
                       ptr_call_str_len, constants_method_ptr_call_str)
                       == 0;
            return is_method_call || is_method_ptr_call;
        };
        if(is_symbol_method_call(marked_source, index)) {
            on_not_assert_start();
            continue;
        }
        const auto open_char_location = marked_source.find(constants_assert_open, index);
        const auto not_found_open_char = open_char_location == std::decay_t<decltype(marked_source)>::npos;
        if(not_found_open_char) {
            on_not_assert_start();
            continue;
        }
        const auto is_symbol_followed_by_whitespace = is_whitespace_or_marked(
            marked_source, constants_non_token,
            index + symbol.length(), open_char_location);
        if(!is_symbol_followed_by_whitespace) {
            on_not_assert_start();
            continue;
        }
        auto open_close_balance = 1;
        index = static_cast<int>(open_char_location);
        while(open_close_balance > 0) {
            ++index;
            if(index >= len) {
                return {
                    .is_ok = false,
                    .error_message = "Unbalanced parenthesis after assertion symbol",
                };
            }
            switch(marked_source[index]) {
            case constants_assert_open:
                ++open_close_balance;
                break;
            case constants_assert_close:
                --open_close_balance;
                break;
            default:
                break;
            }
        }
        assert(marked_source[index] == constants_assert_close);
        const auto end_statement_location =
            marked_source.find(constants_statements_end, index);
        const auto end_statement_not_found =
            end_statement_location == std::decay_t<decltype(marked_source)>::npos;
        if(end_statement_not_found) {
            return {
                .is_ok = false,
                .error_message = "Missing end of statement character after assertion",
            };
        }
        output.push_back(constants_statements_end);
        index = static_cast<int>(end_statement_location + 1);
    }

    return {
        .is_ok = true,
        .content = std::move(output),
    };
}
} // namespace st::ags
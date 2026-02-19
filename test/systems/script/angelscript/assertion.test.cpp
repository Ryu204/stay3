#include <format>
#include <iostream>
#include <catch2/catch_all.hpp>

import stay3;

struct test_case {
    std::string name;
    std::string input;
    bool expected_ok;
    std::string expected_output;
};

const std::vector<test_case> test_cases = {
    {
        "valid.basic.single",
        "assert(false);",
        true,
        ";",
    },
    {
        "valid.basic.empty",
        "",
        true,
        "",
    },
    {
        "valid.basic.multiple",
        "assert(a); assert(b); assert(c);",
        true,
        "; ; ;",
    },
    {
        "valid.basic.whitespace_between",
        "assert  \t  (\n  x > 0  \n  )  \t  ;",
        true,
        ";",
    },
    {
        "valid.basic.empty_parens",
        "assert();",
        true,
        ";",
    },
    {
        "valid.basic.nested_parens",
        "assert((x + y) * (z - w));",
        true,
        ";",
    },
    {
        "valid.basic.string_arg",
        "assert(\"string with (parens)\");",
        true,
        ";",
    },
    {
        "valid.basic.escaped_quotes",
        R"(assert("\"(hello)\"");)",
        true,
        ";",
    },
    {
        "valid.comments.after_paren_before_semicolon",
        "assert(x) /* comment */ ;",
        true,
        ";",
    },
    {
        "valid.comments.inside_arg",
        "assert(/* comment */ x);",
        true,
        ";",
    },
    {
        "valid.comments.between_symbol_and_paren",
        "assert /* comment */ (x);",
        true,
        ";",
    },
    {
        "valid.comments.line_between_symbol_and_paren",
        "assert // comment\n(x);",
        true,
        ";",
    },
    {
        "valid.comments.before_semicolon_multiline",
        "assert(x)  // comment\n  \t  ;",
        true,
        ";",
    },
    {
        "valid.comments.block_after_semicolon",
        "assert(x); /* comment */",
        true,
        "; /* comment */",
    },
    {
        "valid.comments.interleaved_multiple",
        "assert(a); // line\nassert(b); /* block */ assert(c);",
        true,
        "; // line\n; /* block */ ;",
    },
    {
        "valid.protected.line_comment",
        "// assert(x);",
        true,
        "// assert(x);",
    },
    {
        "valid.protected.block_comment",
        "/* assert(x); */",
        true,
        "/* assert(x); */",
    },
    {
        "valid.protected.string_literal",
        "const char* s = \"assert(x);\";",
        true,
        "const char* s = \"assert(x);\";",
    },
    {
        "valid.protected.char_literal",
        "char c = 'a';",
        true,
        "char c = 'a';",
    },
    {
        "valid.protected.raw_string",
        "const char* r = R\"foo(assert(x);)foo\";",
        true,
        "const char* r = R\"foo(assert(x);)foo\";",
    },
    {
        "valid.protected.raw_string_delim_contains_assert",
        "const char* r = R\"delim(assert(42);)delim\";",
        true,
        "const char* r = R\"delim(assert(42);)delim\";",
    },
    {
        "valid.protected.part_of_identifier_prefix",
        "my_assert(x);",
        true,
        "my_assert(x);",
    },
    {
        "valid.protected.part_of_identifier_suffix",
        "assertf(x);",
        true,
        "assertf(x);",
    },
    {
        "valid.protected.part_of_identifier_middle",
        "int assertion = 42;",
        true,
        "int assertion = 42;",
    },
    {
        "valid.protected.as_ptr_method",
        "my_class -> /* comment */ assert(x);",
        true,
        "my_class -> /* comment */ assert(x);",
    },
    {
        "valid.protected.as_method",
        "my_class.assert(x);",
        true,
        "my_class.assert(x);",
    },
    {
        "valid.mixed.comment_and_assert",
        "assert(1); /* assert(2); */ assert(3);",
        true,
        "; /* assert(2); */ ;",
    },
    {
        "valid.mixed.raw_string_then_assert",
        "R\"foo(...)foo\";assert(x);",
        true,
        "R\"foo(...)foo\";;",
    },
    {
        "valid.mixed.multiline_example",
        "int main() {\n    // comment assert(1);\n    assert(x); /* block */ assert(y);\n    assert( (a+b) ); // trailing\n    return 0;\n}",
        true,
        "int main() {\n    // comment assert(1);\n    ; /* block */ ;\n    ; // trailing\n    return 0;\n}",
    },
    {
        "invalid.no_semicolon",
        "assert(x)",
        false,
        "",
    },
    {
        "invalid.unterminated.block_comment",
        "int x; /* assert(x);",
        false,
        "",
    },
    {
        "invalid.unterminated.block_comment_no_close",
        "/* comment",
        false,
        "",
    },
    {
        "invalid.unterminated.string_literal",
        "const char* s = \"hello;",
        false,
        "",
    },
    {
        "invalid.unterminated.char_literal",
        "char c = 'a;",
        false,
        "",
    },
    {
        "invalid.unterminated.char_literal_escape",
        "char c = '\\';",
        false,
        "",
    },
    {
        "invalid.unterminated.raw_string_missing_delim",
        "R\"foo(abc)bar\"",
        false,
        "",
    },
    {
        "invalid.unterminated.raw_string_no_closing_quote",
        "R\"foo(abc)",
        false,
        "",
    },
};

TEST_CASE("All cases") {
    for(auto &&[name, input, expected_ok, expected_output]: test_cases) {
        const auto fmt = [&name]<typename type>(type &&inp) -> std::string {
            return std::format("[{}]{}", name, std::forward<type>(inp));
        };
        const auto stripped = st::ags::strip_assert_statements({.source = input});
        if(stripped.is_ok != expected_ok) {
            std::cout << stripped.error_message.value_or("no error message") << '\n';
        }
        REQUIRE(fmt(stripped.is_ok) == fmt(expected_ok));
        if(expected_ok) {
            REQUIRE(stripped.content.has_value());
            REQUIRE(fmt(stripped.content.value()) == fmt(expected_output));
        }
    }
}

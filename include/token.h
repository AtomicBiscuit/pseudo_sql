#pragma once

#include "syexception.h"
#include "operations.h"

#include <memory>
#include <map>
#include <vector>
#include <ranges>
#include <algorithm>

using std::string_literals::operator ""s;

namespace tokenize {
    void skip_spaces(std::string_view &view, bool reversed = false);

    bool check_empty(const std::string_view &view);

    std::string get_str(std::string_view &);

    std::vector<bool> get_bytes(std::string_view &);

    int get_int(std::string_view &);

    std::string get_full_name(std::string_view &view);

    std::string get_name(std::string_view &view);

    enum class Type {
        UnaryOperation, BinaryOperation,
        Operand,
        OpenScope, CloseScope,
        Len
    };

    struct Token {
        std::unique_ptr<database::Operation> oper;
        Type type;
        int prior;

        Token(std::unique_ptr<database::Operation> s, Type t, int prior) : oper(std::move(s)), type(t), prior(prior) {};

        Token(Token &&other) noexcept: oper(std::move(other.oper)), type(other.type), prior(other.prior) {};

        Token &operator=(Token &&other) noexcept;

        static Token get_operation(std::string_view &c, Type last, bool is_len);
    };
}
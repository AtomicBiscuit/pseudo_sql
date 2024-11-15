#pragma once

#include <memory>
#include <map>
#include "table.h"
#include "syexception.h"
#include "operations.h"


namespace tokenize {
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

        static std::string get_str(std::string_view &);

        static std::vector<bool> get_bytes(std::string_view &);

        static int get_int(std::string_view &);

        static std::string get_full_name(std::string_view &view);
        static std::string get_name(std::string_view &view);
    };
}
#include "../include/token.h"
#include <string>

using namespace tokenize;
using namespace std::string_literals;

Token Token::get_operation(std::string_view &c, Type last, bool is_len) {
    if ("+-!"s.contains(c[0]) and last != Type::Operand and last != Type::UnaryOperation) {
        char op = c[0];
        c.remove_prefix(1);
        return {std::make_unique<database::UnaryOperation>(op), Type::UnaryOperation, 5};
    } else if ("+-"s.contains(c[0]) and last == Type::Operand) {
        char op = c[0];
        c.remove_prefix(1);
        return {std::make_unique<database::BinaryOperation>(op), Type::BinaryOperation, 1};
    } else if ("*/%"s.contains(c[0]) and last == Type::Operand) {
        char op = c[0];
        c.remove_prefix(1);
        return {std::make_unique<database::BinaryOperation>(op), Type::BinaryOperation, 2};
    }

    if (c.starts_with(">=") or c.starts_with("<=") or c.starts_with("!=") and last == Type::Operand) {
        auto op = std::string(c.begin(), c.begin() + 2);
        c.remove_prefix(2);
        return {std::make_unique<database::ComparisonOperation>(op), Type::BinaryOperation, 0};
    } else if (">=<"s.contains(c[0]) and last == Type::Operand) {
        auto op = std::string(c[0], 1);
        c.remove_prefix(1);
        return {std::make_unique<database::ComparisonOperation>(op), Type::BinaryOperation, 0};
    }

    if (c[0] == '(' and last != Type::Operand) {
        c.remove_prefix(1);
        return {std::make_unique<database::ScopeOperation>(), Type::OpenScope, 10};
    } else if (c[0] == ')' and last == Type::Operand) {
        c.remove_prefix(1);
        return {nullptr, Type::CloseScope, 10};
    }

    if (c[0] == '|' and last != Type::Operand and !is_len) {
        c.remove_prefix(1);
        return {std::make_unique<database::LenOperation>(), Type::Len, 10};
    } else if (c[0] == '|' and is_len) {
        c.remove_prefix(1);
        return {nullptr, Type::Len, 10};
    } else if ((c.starts_with("&&") or c.starts_with("||") or c.starts_with("^^")) and last == Type::Operand) {
        char op = c[0];
        int prior = op == '&' ? -1 : -2;
        c.remove_prefix(2);
        return {std::make_unique<database::BinaryOperation>(op), Type::BinaryOperation, prior};
    }

    if (c[0] == '"' and last != Type::Operand) {
        return {std::make_unique<database::StringOperation>(get_str(c)), Type::Operand, 20};
    } else if (c.starts_with("0x") and last != Type::Operand) {
        return {std::make_unique<database::BytesOperation>(get_bytes(c)), Type::Operand, 20};
    } else if (isdigit(c[0]) and last != Type::Operand) {
        return {std::make_unique<database::IntegerOperation>(get_int(c)), Type::Operand, 20};
    } else if (c.starts_with("false") and not(c.length() >= 6 and (isalnum(c[5]) or c[5] == '.' or c[5] == '_')) and
               last != Type::Operand) {
        c.remove_prefix(5);
        return {std::make_unique<database::BoolOperation>(false), Type::Operand, 20};
    } else if (c.starts_with("true") and not(c.length() >= 5 and (isalnum(c[4]) or c[4] == '.' or c[4] == '_')) and
               last != Type::Operand) {
        c.remove_prefix(4);
        return {std::make_unique<database::BoolOperation>(true), Type::Operand, 20};
    } else if (isalpha(c[0]) and last != Type::Operand) {
        return {std::make_unique<database::FieldOperation>(get_name(c)), Type::Operand, 42};
    }

    throw syntax_error("Неразрешимый в контексте литерал: " + std::string(c));
}

std::string Token::get_str(std::string_view &view) {
    int cnt = 1;
    auto c = view.begin();
    c++;
    while (c != view.end() and *c != '"') {
        if (*(c++) == '\\') {
            ++cnt;
            if ((++c) == view.end()) {
                break;
            }
        }
        ++cnt;
    }
    if (c == view.end()) {
        throw syntax_error("Не найден конец строки");
    }
    auto temp = std::string(view.begin() + 1, view.begin() + cnt);
    view.remove_prefix(cnt + 1);
    return std::move(temp);
}

std::vector<bool> Token::get_bytes(std::string_view &view) {
    int cnt = 2;
    std::vector<bool> b;
    auto c = view.begin() + 2;
    while (c != view.end() and isxdigit(static_cast<unsigned char>(*c))) {
        auto C = static_cast<unsigned char>(*c);
        int n = (isdigit(C) ? C - '0' : tolower(C) - 'a' + 10);
        b.push_back(n / 8);
        b.push_back(n / 4 % 2);
        b.push_back(n / 2 % 2);
        b.push_back(n % 2);
        ++cnt;
        ++c;
    }
    view.remove_prefix(cnt);
    return std::move(b);
}

Token &Token::operator=(Token &&other) noexcept {
    oper = std::move(other.oper);
    prior = other.prior;
    type = other.type;
    return *this;
}

int Token::get_int(std::string_view &view) {
    int cnt = 0;
    int b = 0;
    auto c = view.begin();
    while (c != view.end() and isdigit(static_cast<unsigned char>(*c))) {
        b = 10 * b + *c - '0';
        ++c;
        ++cnt;
    }
    view.remove_prefix(cnt);
    return b;
}

std::string Token::get_name(std::string_view &view) {
    int cnt = 0;
    auto c = view.begin();
    while (c != view.end() and (isalnum(*c) or *c == '.' or *c == '_')) {
        ++c;
        ++cnt;
    }
    auto temp = std::string(view.begin(), view.begin() + cnt);
    view.remove_prefix(cnt);
    return std::move(temp);
}

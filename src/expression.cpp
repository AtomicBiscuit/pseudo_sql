#include <tuple>
#include <stack>
#include "../include/expression.h"

using namespace database;

static void
pop_safe(std::vector<tokenize::Token> &stack, std::vector<std::unique_ptr<Operation>> &stack_dest,
         const std::string &err) {
    if (stack.empty()) {
        throw syntax_error(err);
    }
    stack_dest.push_back(std::move(stack.back().oper));
    stack.pop_back();
}

std::unique_ptr<database::Operation> ColumnExpression::parse(const std::string &in, ColumnContext &ctx) {
    tokenize::Token cur = {nullptr, tokenize::Type::BinaryOperation, 0};
    bool is_len = false;
    std::vector<std::unique_ptr<Operation>> post;
    std::vector<tokenize::Token> ops;
    auto view = std::string_view(in);
    while (!view.empty()) {
        if (isspace(view[0])) {
            view.remove_prefix(1);
            continue;
        }
        cur = std::move(tokenize::Token::get_operation(view, cur.type, is_len));

        if (cur.prior == 42) {
            dynamic_cast<FieldOperation *>(cur.oper.get())->resolve(ctx);
        }

        if (cur.type == tokenize::Type::Operand) {
            post.push_back(std::move(cur.oper));
        } else if (cur.type == tokenize::Type::Len and is_len) {
            is_len = false;
            cur.type = tokenize::Type::Operand;
            if (ops.empty()) {
                throw syntax_error("Отсутствует аргумент оператора `|`");
            }
            while (ops.back().type != tokenize::Type::Len) {
                if (ops.back().type == tokenize::Type::OpenScope) {
                    throw syntax_error("Некорректный порядок скобок и оператора длины `|`");
                }
                pop_safe(ops, post, "Невозможная скобочная последовательность, возможно не хватает `|`");
            }
            pop_safe(ops, post, "Невозможная скобочная последовательность, возможно не хватает `|`");
        } else if (cur.type == tokenize::Type::Len) {
            is_len = true;
            ops.push_back(std::move(cur));
        } else if (cur.type == tokenize::Type::OpenScope) {
            ops.push_back(std::move(cur));
        } else if (cur.type == tokenize::Type::CloseScope) {
            cur.type = tokenize::Type::Operand;
            if (ops.empty()) {
                throw syntax_error("Обнаружено пустое подвыражение");
            }
            while (ops.back().type != tokenize::Type::OpenScope) {
                if (ops.back().type == tokenize::Type::Len) {
                    throw syntax_error("Некорректный порядок скобок и оператора длины `|`");
                }
                pop_safe(ops, post, "Невозможная скобочная последовательность, возможно не хватает `(`");
            }
            ops.pop_back();
        } else if (cur.type == tokenize::Type::BinaryOperation or cur.type == tokenize::Type::UnaryOperation) {
            while (not ops.empty() and ops.back().prior >= cur.prior and (
                    ops.back().type == tokenize::Type::BinaryOperation or
                    ops.back().type == tokenize::Type::UnaryOperation)) {
                pop_safe(ops, post, "Непредвиденная ошибка");
            }
            ops.push_back(std::move(cur));
        } else {
            throw syntax_error("Непредвиденная ошибка");
        }
    }
    while (not ops.empty()) {
        if (ops.back().type == tokenize::Type::OpenScope or ops.back().type == tokenize::Type::Len) {
            throw syntax_error("Невозможная скобочная последовательность, возможно не хватает `)` или '|'");
        }
        pop_safe(ops, post, "Непредвиденная ошибка");
    }
    if (post.empty()) {
        throw syntax_error("Найдено пустое выражение");
    }
    std::unique_ptr<Operation> result = std::move(post.back());
    post.pop_back();
    result->bind(post);
    if (not post.empty()) {
        throw syntax_error("Невалидное число операндом в выражении");
    }
    return result;
}
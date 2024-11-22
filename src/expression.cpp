#include <tuple>
#include "../include/expression.h"
#include "../include/token.h"
#include "../include/select.h"
#include "../include/db.h"

using namespace database;
using namespace operations;
using namespace std::string_literals;

static void pop_safe(std::vector<PrioritizedOperation> &stack,
                     std::vector<std::unique_ptr<Operation>> &stack_dest,
                     const std::string &err) {
    SYNTAX_ASSERT(!stack.empty(), err);
    stack_dest.push_back(std::move(stack.back().oper_));
    stack.pop_back();
}


std::tuple<std::shared_ptr<Table>, std::string>
database::get_table_from_expression(const std::string &in, TableContext &ctx) {
    auto view = std::string_view(in);
    std::string alias;
    std::shared_ptr<Table> table;
    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(!view.empty(), "Ожидалось табличное выражение");

    auto parts = tokenize::clear_parse(std::string(view), "as", true);
    auto table_view = std::string_view(parts[0]);
    SYNTAX_ASSERT(parts.size() <= 2, "Обнаружено несколько вхождений ключевого слова `as`");

    if (table_view[0] != '(') {
        alias = tokenize::get_name(table_view);
        EXEC_ASSERT(ctx.tables.contains(alias), "Таблица `" + alias + "` не найдена");
        table = ctx.tables[alias];
    } else {
        table_view.remove_prefix(1);
        tokenize::skip_spaces(table_view, true);
        SYNTAX_ASSERT(table_view.ends_with(')'), "Невозможная скобочная последовательность, возможно пропущена`)`");

        table_view.remove_suffix(1);

        table = Select().parse_and_execute(std::string(table_view), ctx);
        alias = table->name();
    }
    if (parts.size() == 2) {
        auto view_alias = std::string_view(parts[1]);
        alias = tokenize::get_name(view_alias);
        SYNTAX_ASSERT(tokenize::check_empty(view_alias), "Непредвиденный литерал `" + std::string(view_alias) + "`");
    }
    return {table, alias};
}


std::unique_ptr<Operation>
database::build_execution_tree_from_expression(const std::string &in, ColumnContext &ctx) {
    PrioritizedOperation cur = {nullptr, OperationType::BinaryOperation, 0};
    bool is_len = false;
    std::vector<std::unique_ptr<Operation>> post;
    std::vector<PrioritizedOperation> ops;
    auto view = std::string_view(in);
    while (!view.empty()) {
        if (isspace(view[0])) {
            view.remove_prefix(1);
            continue;
        }
        cur = PrioritizedOperation::get_operation(view, cur.type_, is_len);

        if (cur.prior_ == 42) {
            dynamic_cast<FieldOperation *>(cur.oper_.get())->resolve(ctx);
        }

        if (cur.type_ == OperationType::Operand) {
            post.push_back(std::move(cur.oper_));
        } else if (cur.type_ == OperationType::Len and is_len) {
            is_len = false;
            cur.type_ = OperationType::Operand;
            SYNTAX_ASSERT(!ops.empty(), "Отсутствует аргумент оператора `Len`");
            while (ops.back().type_ != OperationType::Len) {
                SYNTAX_ASSERT(ops.back().type_ != OperationType::OpenScope,
                              "Некорректный порядок скобок и оператора длины `|`");
                pop_safe(ops, post, "Невозможная скобочная последовательность, возможно не хватает `|`");
            }
            pop_safe(ops, post, "Невозможная скобочная последовательность, возможно не хватает `|`");
        } else if (cur.type_ == OperationType::Len) {
            is_len = true;
            ops.push_back(std::move(cur));
        } else if (cur.type_ == OperationType::OpenScope) {
            ops.push_back(std::move(cur));
        } else if (cur.type_ == OperationType::CloseScope) {
            cur.type_ = OperationType::Operand;
            SYNTAX_ASSERT(!ops.empty(), "Невозможная скобочная последовательность, возможно не хватает `(`");
            while (ops.back().type_ != OperationType::OpenScope) {
                SYNTAX_ASSERT(ops.back().type_ != OperationType::Len,
                              "Некорректный порядок скобок и оператора длины `|`");
                pop_safe(ops, post, "Невозможная скобочная последовательность, возможно не хватает `(`");
                SYNTAX_ASSERT(!ops.empty(), "Невозможная скобочная последовательность, возможно не хватает `(`");
            }
            ops.pop_back();
        } else if (cur.type_ == OperationType::BinaryOperation or
                   cur.type_ == OperationType::UnaryOperation) {
            while (not ops.empty() and ops.back().prior_ >= cur.prior_ and (
                    ops.back().type_ == OperationType::BinaryOperation or
                    ops.back().type_ == OperationType::UnaryOperation)) {
                pop_safe(ops, post, "Непредвиденная ошибка");
            }
            ops.push_back(std::move(cur));
        } else {
            SYNTAX_ASSERT(0, "Непредвиденная ошибка");
        }
    }
    while (not ops.empty()) {
        SYNTAX_ASSERT(ops.back().type_ != OperationType::OpenScope and ops.back().type_ != OperationType::Len,
                      "Невозможная скобочная последовательность, возможно не хватает `)` или '|'");
        pop_safe(ops, post, "Непредвиденная ошибка");
    }
    SYNTAX_ASSERT(!post.empty(), "Найдено пустое выражение");

    std::unique_ptr<Operation> result = std::move(post.back());
    post.pop_back();
    result->bind(post);
    SYNTAX_ASSERT(post.empty(), "Невалидное число операндов в выражении");
    return result;
}
#include <tuple>
#include "../include/expression.h"
#include "../include/token.h"
#include "../include/select.h"
#include "../include/db.h"

using namespace database;
using namespace operations;
using namespace std::string_literals;
using
enum OperationType;

static void pop_safe(std::vector<PrioritizedOperation> &source, std::vector<std::unique_ptr<Operation>> &dest,
                     const std::string &err) {
    SYNTAX_ASSERT(!source.empty(), err);
    SYNTAX_ASSERT(source.back().type_ != OpenScope and source.back().type_ != Len,
                  "Некорректный порядок скобок `(` и оператора длины `|`");
    dest.push_back(std::move(source.back().oper_));
    source.pop_back();
}


static void push_operation(PrioritizedOperation &operation, bool &is_len, std::vector<PrioritizedOperation> &infix,
                           std::vector<std::unique_ptr<Operation>> &postfix) {
    if (operation.type_ == Operand) {
        postfix.push_back(std::move(operation.oper_));
    }
    if (operation.type_ == Len and is_len) {
        is_len = false;
        operation.type_ = Operand;

        while (not infix.empty() and infix.back().type_ != Len) {
            pop_safe(infix, postfix, "Невозможная скобочная последовательность, возможно не хватает `|`");
        }
        SYNTAX_ASSERT(!infix.empty(), "Невозможная скобочная последовательность, возможно не хватает `|`");

        postfix.push_back(std::move(infix.back().oper_));
        infix.pop_back();
    } else if (operation.type_ == Len) {
        is_len = true;
        infix.push_back(std::move(operation));
    }
    if (operation.type_ == OpenScope) {
        infix.push_back(std::move(operation));
    }
    if (operation.type_ == CloseScope) {
        operation.type_ = Operand;

        while (!infix.empty() and infix.back().type_ != OpenScope) {
            pop_safe(infix, postfix, "Невозможная скобочная последовательность, возможно не хватает `(`");
        }
        SYNTAX_ASSERT(!infix.empty(), "Невозможная скобочная последовательность, возможно не хватает `(`");

        infix.pop_back();
    }
    if (operation.type_ == Binary or operation.type_ == Unary) {
        while (not infix.empty() and infix.back().prior_ >= operation.prior_) {
            pop_safe(infix, postfix, "Непредвиденная ошибка");
        }
        infix.push_back(std::move(operation));
    }
}


std::tuple<Table, std::string> database::get_table_from_expression(const std::string &str, TableContext &ctx) {
    auto view = std::string_view(str);
    std::string alias;
    Table table{};

    auto parts = tokenize::clear_parse(std::string(view), "as", true);
    auto table_view = std::string_view(parts[0]);
    SYNTAX_ASSERT(parts.size() <= 2, "Обнаружено несколько вхождений ключевого слова `as`");

    tokenize::skip_spaces(table_view);
    if (not table_view.starts_with("(")) {
        alias = tokenize::get_name(table_view);
        EXEC_ASSERT(ctx.tables.contains(alias), "Таблица `" + alias + "` не найдена");
        table = ctx.tables[alias];
    } else {
        table_view.remove_prefix(1);

        tokenize::skip_spaces(table_view, true);
        SYNTAX_ASSERT(table_view.ends_with(')'), "Невозможная скобочная последовательность, возможно пропущена`)`");
        table_view.remove_suffix(1);

        table = Select().parse_and_execute(std::string(table_view), ctx);
        alias = table.name();
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
    PrioritizedOperation cur = {nullptr, Binary, 0};
    bool is_len = false;
    std::vector<std::unique_ptr<Operation>> postfix;
    std::vector<PrioritizedOperation> infix;

    auto view = std::string_view(in);

    while (!view.empty()) {
        if (isspace(view[0])) {
            tokenize::skip_spaces(view);
            continue;
        }
        cur = PrioritizedOperation::get_operation(view, cur.type_, is_len);

        if (cur.prior_ == 42) { // Приоритет 42 только у FieldExpression
            dynamic_cast<FieldOperation *>(cur.oper_.get())->resolve(ctx);
        }

        push_operation(cur, is_len, infix, postfix);
    }
    while (not infix.empty()) {
        pop_safe(infix, postfix, "Невозможная скобочная последовательность, возможно не хватает `)` или '|'");
    }
    SYNTAX_ASSERT(!postfix.empty(), "Найдено пустое выражение");

    std::unique_ptr<Operation> result = std::move(postfix.back());
    postfix.pop_back();
    result->bind(postfix);
    SYNTAX_ASSERT(postfix.empty(), "Невалидное число операндов в выражении");
    return result;
}
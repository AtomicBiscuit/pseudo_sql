#include "../include/update.h"
#include "../include/token.h"
#include "../include/column.h"

using namespace database;
using namespace operations;

Table Update::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string word;

    SYNTAX_ASSERT(tokenize::to_lower(word = tokenize::get_word(view)) == "update",
                  "Ожидалось ключевое слово `update`, найдено: " + word);

    auto name = tokenize::get_name(view);
    SYNTAX_ASSERT(tokenize::to_lower(word = tokenize::get_word(view)) == "set",
                  "Ожидалось ключевое слово `set`, найдено: " + word);
    EXEC_ASSERT(ctx.tables.contains(name), "Таблица с именем `" + name + "` не найдена");

    auto table = ctx.tables[name];
    ColumnContext column_ctx;
    table.add_columns_to_context(table.name(), column_ctx, 0, table.get_columns().size(), true);

    auto assignments_other = tokenize::clear_parse(std::string(view), "where", true);
    SYNTAX_ASSERT(assignments_other.size() == 2, "Ключевое слово `where` должно единожды встречаться в запросе");

    auto condition = build_execution_tree_from_expression(assignments_other[1], column_ctx);

    EXEC_ASSERT(condition->type() == Type::Boolean,
                "Тип выражения `where`(" + type_to_str(condition->type()) + ") не является bool");

    auto assignments = _parse_assignments(assignments_other[0], column_ctx);

    return update(table, condition, assignments);
}

std::vector<std::pair<std::shared_ptr<IColumn>, std::unique_ptr<Operation>>>
Update::_parse_assignments(const std::string &str, ColumnContext &ctx) {
    std::vector<std::pair<std::shared_ptr<IColumn>, std::unique_ptr<Operation>>> res;
    auto parts = tokenize::clear_parse(str, ",", false);

    for (const auto &assign_expr: parts) {
        auto table_other = tokenize::clear_parse(assign_expr, "=", false);
        SYNTAX_ASSERT(table_other.size() == 2, "Некорректный формат выражения после ключевого слова `set`");

        auto col_view = std::string_view(table_other[0]);
        auto col_name = tokenize::get_name(col_view);
        SYNTAX_ASSERT(tokenize::check_empty(col_view), "Непредвиденный литерал: " + std::string(col_view));
        EXEC_ASSERT(ctx.contains(col_name), "Таблица `" + col_name + "` не найдена");

        auto column = ctx[col_name];
        auto operation = build_execution_tree_from_expression(table_other[1], ctx);
        res.emplace_back(column, std::move(operation));
    }
    return res;
}

Table Update::update(Table table, std::unique_ptr<operations::Operation> &condition,
                     std::vector<std::pair<std::shared_ptr<IColumn>, std::unique_ptr<operations::Operation>>> &assign) {
    std::map<std::string, bool> visited;
    for (auto &[col, oper]: assign) {
        EXEC_ASSERT(!visited.contains(col->name()), "Столбец `" + col->name() + "` изменяется несколько раз");
        EXEC_ASSERT(col->type() == oper->type(),
                    "Тип столбца(" + type_to_str(col->type()) + ") не совпадает с типом выражения(" +
                    type_to_str(oper->type()) + ")");
        visited[col->name()] = true;
    }

    std::vector<int> valid_rows;
    int size = table.get_columns().empty() ? 0 : table.get_columns()[0]->size();
    for (int i = 0; i < size; i++) {
        if (get<bool>(condition->eval(i))) {
            valid_rows.push_back(i);
        }
    }
    for (auto &[col, oper]: assign) {
        std::vector<value_t> values;
        values.reserve(valid_rows.size());
        for (auto i: valid_rows) {
            values.push_back(oper->eval(i));
        }
        col->apply_update(valid_rows, values);
    }
    table.check_valid();
    return table;
}
#include "../../include/command/delete.h"
#include "../../include/token.h"

using namespace database;
using namespace operations;

Table Delete::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string word;

    SYNTAX_ASSERT(tokenize::to_lower(word = tokenize::get_word(view)) == "delete",
                  "Ожидалось ключевое слово `delete`, найдено: " + word);

    auto name = tokenize::get_name(view);

    SYNTAX_ASSERT(tokenize::to_lower(word = tokenize::get_word(view)) == "where",
                  "Ожидалось ключевое слово `where`, найдено: " + word);
    EXEC_ASSERT(ctx.tables.contains(name), "Таблица с именем `" + name + "` не найдена");

    auto table = ctx.tables[name];
    ColumnContext column_ctx;
    table.add_columns_to_context(table.name(), column_ctx, 0, table.columns().size(), true);

    auto condition = build_execution_tree_from_expression(std::string(view));

    condition->resolve(column_ctx);

    EXEC_ASSERT(condition->type() == Type::Bool,
                "Тип выражения `where`(" + type_to_str(condition->type()) + ") не является bool");

    return delete_impl(table, condition);
}

Table Delete::delete_impl(Table table, std::unique_ptr<Operation> &condition) {
    std::vector<int> valid;
    auto cols = table.columns();
    int size = cols.empty() ? 0 : cols[0]->size();
    for (int i = 0; i < size; i++) {
        if (not get<bool>(condition->eval(i))) {
            valid.push_back(i);
        }
    }
    for (auto &col: cols) {
        col->apply_delete(valid);
    }
    return table;
}
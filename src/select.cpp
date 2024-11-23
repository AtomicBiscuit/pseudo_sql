#include <sstream>
#include <vector>
#include <ranges>
#include "../include/select.h"
#include "../include/token.h"

using namespace std::string_literals;
using namespace database;
using namespace operations;

template<typename T>
requires(std::convertible_to<T, value_t>)
static std::shared_ptr<IColumn>
impl_fill(const std::vector<int> &valid, std::unique_ptr<Operation> val, const std::string &name) {
    std::vector<T> vals;
    vals.reserve(valid.size());
    for (auto i: valid) {
        vals.push_back(std::move(get<T>(val->eval(i))));
    }
    auto temp = std::make_shared<Column<T>>(val->type(), name);
    temp->set_rows(std::move(vals));
    return temp;
}

static std::shared_ptr<IColumn>
fill(const std::vector<int> &valid, std::unique_ptr<Operation> val, const std::string &name) {
    if (val->type() == Type::Integer) {
        return impl_fill<int>(valid, std::move(val), name);
    } else if (val->type() == Type::Boolean) {
        return impl_fill<bool>(valid, std::move(val), name);
    } else if (val->type() == Type::String) {
        return impl_fill<std::string>(valid, std::move(val), name);
    } else if (val->type() == Type::Bytes) {
        return impl_fill<std::vector<bool>>(valid, std::move(val), name);
    }
}


Table Select::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string temp;
    SYNTAX_ASSERT(tokenize::to_lower(temp = tokenize::get_word(view)) == "select",
                  "Ожидалось ключевое слово `select`, найдено: " + temp);

    auto cols_other = tokenize::clear_parse(std::string(view), "from", true);
    SYNTAX_ASSERT(cols_other.size() == 2, "Ключевое слово `from` должно единожды встречаться в запросе");

    auto table_other = tokenize::clear_parse(cols_other[1], "where", true);
    SYNTAX_ASSERT(table_other.size() == 2, "Ключевое слово `where` должно единожды встречаться в запросе");

    auto [table, column_ctx] = _resolve_table_expr(table_other[0], ctx);

    auto columns = _resolve_column_expr(cols_other[0], column_ctx);

    auto condition = build_execution_tree_from_expression(table_other[1], column_ctx);

    EXEC_ASSERT(condition->type() == Type::Boolean,
                "Тип выражения `where`(" + type_to_str(condition->type()) + ") не является bool");

    return select(table, std::move(condition), columns, "Table" + std::to_string(++ctx.number));
}

std::vector<std::pair<std::unique_ptr<Operation>, std::string>>
Select::_resolve_column_expr(const std::string &cols, ColumnContext &ctx) {
    std::vector<std::pair<std::unique_ptr<Operation>, std::string>> columns{};
    int col_num = 0;

    for (auto &full_column: tokenize::clear_parse(cols, ",", false)) {
        auto parts = tokenize::clear_parse(full_column, "as", true);
        SYNTAX_ASSERT(parts.size() <= 2,
                      "Ключевое слово `as` должно не более одного раза встречаться в конце выражения");

        auto col_expr = build_execution_tree_from_expression(parts[0], ctx);
        std::string name = "column" + std::to_string(++col_num);

        if (parts.size() == 1) {
            FieldOperation *column;
            if ((column = dynamic_cast<FieldOperation *>(col_expr.get()))) {
                name = tokenize::clear_parse(column->name(), ".", false).back();
            }
        } else {
            auto view = std::string_view(parts[1]);
            name = tokenize::get_name(view);\
            SYNTAX_ASSERT(tokenize::check_empty(view), "Непредвиденный литерал `" + std::string(view) + "`");
        }
        columns.emplace_back(std::move(col_expr), name);
    }
    return columns;
}

void
Select::add_columns_to_context(Table &table, const std::string &name, int from, int count, bool shorty,
                               ColumnContext &ctx) {
    auto cols = table.get_columns();
    for (auto col: std::views::counted(cols.begin() + from, count)) {
        auto full_name = name + "." + col->name();
        EXEC_ASSERT(!ctx.contains(full_name), "Неоднозначно определен столбец `" + full_name + "`");
        ctx[full_name] = col;
        if (shorty) {
            ctx.emplace(col->name(), col);
        }
    }
}

void Select::select(Table &table, std::unique_ptr<Operation> cond) {
    std::vector<int> row_nums;
    int rows = table.get_columns().empty() ? 0 : table.get_columns()[0]->size();
    for (int i = 0; i < rows; ++i) {
        if (std::get<bool>(cond->eval(i))) {
            row_nums.push_back(i);
        }
    }
    for (auto &col: table.get_columns()) {
        col->apply_delete(row_nums);
    }
}

Table Select::select(Table &table, std::unique_ptr<Operation> condition,
                     std::vector<std::pair<std::unique_ptr<Operation>, std::string>> &columns,
                     const std::string &table_name) {
    std::vector<int> valid;
    int row_cnt = static_cast<int>(table.get_columns().empty() ? 0 : table.get_columns()[0]->size());
    for (int i = 0; i < row_cnt; ++i) {
        if (get<bool>(condition->eval(i))) {
            valid.push_back(i);
        }
    }

    auto result_table = Table(table_name);
    for (auto &[col, name]: columns) {
        result_table.add_column(fill(valid, std::move(col), name));
    }
    return result_table;
}

Table Select::cartesian_product(const Table &table1, const Table &table2) {
    auto cols1 = table1.get_columns();
    auto cols2 = table2.get_columns();
    auto res = Table(table1.name() + "_" + table2.name());
    size_t sz1 = (cols1.empty() ? 0 : cols1[0]->size());
    size_t sz2 = (cols2.empty() ? 0 : cols2[0]->size());
    for (size_t i = 0; i < cols1.size(); i++) {
        res.add_column(cols1[i]->multicopy(sz2, false));
    }
    for (size_t i = 0; i < cols2.size(); i++) {
        res.add_column(cols2[i]->multicopy(sz1, true));
    }
    return res;
}

std::tuple<Table, ColumnContext> Select::_resolve_table_expr(const std::string &table_str, TableContext &ctx) {
    auto parts = tokenize::clear_parse(table_str, "join", true);

    std::vector<std::pair<std::string, int>> history;
    auto [table, alias] = get_table_from_expression(parts[0], ctx);

    ColumnContext column_ctx;
    add_columns_to_context(table, alias, 0, table.get_columns().size(), true, column_ctx);
    history.push_back({alias, table.get_columns().size()});

    for (auto it = parts.begin() + 1; it < parts.end(); ++it) {
        auto subparts = tokenize::clear_parse(*it, "on", true);
        SYNTAX_ASSERT(subparts.size() == 2, "Некорректное использование ключевого слова `on`");

        auto [cur_table, cur_alias] = get_table_from_expression(subparts[0], ctx);
        history.push_back({cur_alias, cur_table.get_columns().size()});

        table = cartesian_product(table, cur_table);

        column_ctx.clear();
        int from = 0;
        for (auto &[al, cnt]: history) {
            add_columns_to_context(table, al, from, cnt, false, column_ctx);
            from += cnt;
        }
        auto condition = build_execution_tree_from_expression(subparts[1], column_ctx);
        EXEC_ASSERT(condition->type() == Type::Boolean,
                    "Тип выражения `on`(" + type_to_str(condition->type()) + ") не является bool");
        select(table, std::move(condition));
    }
    return {table, column_ctx};
}
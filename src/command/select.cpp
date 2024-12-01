#include <sstream>
#include <vector>
#include <ranges>
#include "../../include/command/select.h"
#include "../../include/token.h"

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
    } else if (val->type() == Type::Bool) {
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

    auto condition = build_execution_tree_from_expression(table_other[1]);

    condition->resolve(column_ctx);

    EXEC_ASSERT(condition->type() == Type::Bool,
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

        auto col_expr = build_execution_tree_from_expression(parts[0]);

        col_expr->resolve(ctx);

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

std::unique_ptr<operations::Operation>
Select::_save_on_condition(Table &table, std::unique_ptr<operations::Operation> cond) {
    std::vector<int> row_nums;
    int rows = table.columns().empty() ? 0 : table.columns()[0]->size();
    for (int i = 0; i < rows; ++i) {
        if (std::get<bool>(cond->eval(i))) {
            row_nums.push_back(i);
        }
    }
    for (auto &col: table.columns()) {
        col->apply_delete(row_nums);
    }
    return std::move(cond);
}

Table Select::select(Table &table, std::unique_ptr<Operation> condition,
                     std::vector<std::pair<std::unique_ptr<Operation>, std::string>> &columns,
                     const std::string &table_name) {
    std::vector<int> valid;
    int row_cnt = static_cast<int>(table.columns().empty() ? 0 : table.columns()[0]->size());
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

void Select::cartesian_product_on_condition(Table &res, const Table &table1, const Table &table2,
                                            std::unique_ptr<Operation> cond) {
    auto cols = res.columns();
    auto cols1 = table1.columns();
    auto cols2 = table2.columns();
    auto temp = Table(res.name());
    for (size_t i = 0; i < cols.size(); i++) {
        temp.add_column(cols[i]->copy_empty());
    }
    int sz1 = int(cols1.empty() ? 0 : cols1[0]->size());
    int sz2 = int(cols2.empty() ? 0 : cols2[0]->size());

    int k = std::max(1, sz2 / 100);  /* TODO Адаптивно подбирать k
                                            Чем меньше k тем меньше памяти занимает join
                                            Но и больше лишних операций записи происходит */
    for (int i = 0; i < sz2; i += k) {
        for (int c1 = 0; c1 < cols1.size(); c1++) {
            cols1[c1]->copy_to(std::min(k, sz2 - i), cols[c1]);
        }
        for (int c2 = 0; c2 < cols2.size(); c2++) {
            cols2[c2]->copy_to(i, sz1, std::min(k, sz2 - i), cols[cols1.size() + c2]);
        }
        cond = _save_on_condition(res, std::move(cond));
        temp.merge(res);
    }
    res.merge(temp);
}

std::tuple<Table, ColumnContext> Select::_resolve_table_expr(const std::string &table_str, TableContext &ctx) {
    auto parts = tokenize::clear_parse(table_str, "join", true);

    std::vector<std::pair<std::string, int>> history;
    auto [res, alias] = get_table_from_expression(parts[0], ctx);

    ColumnContext column_ctx;
    res.add_columns_to_context(alias, column_ctx, 0, res.columns().size(), true);
    history.push_back({alias, res.columns().size()});

    for (auto it = parts.begin() + 1; it < parts.end(); ++it) {
        auto subparts = tokenize::clear_parse(*it, "on", true);
        SYNTAX_ASSERT(subparts.size() == 2, "Некорректное использование ключевого слова `on`");

        auto [cur_table, cur_alias] = get_table_from_expression(subparts[0], ctx);
        history.push_back({cur_alias, cur_table.columns().size()});

        auto condition = build_execution_tree_from_expression(subparts[1]);

        Table last_table{std::move(res)};

        auto cols1 = last_table.columns();
        auto cols2 = cur_table.columns();

        res = Table(last_table.name() + "_" + cur_table.name());
        for (size_t i = 0; i < cols1.size(); i++) {
            res.add_column(cols1[i]->copy_empty());
        }
        for (size_t i = 0; i < cols2.size(); i++) {
            res.add_column(cols2[i]->copy_empty());
        }

        column_ctx.clear();
        int from = 0;
        for (auto &[al, cnt]: history) {
            res.add_columns_to_context(al, column_ctx, from, cnt, false);
            from += cnt;
        }

        condition->resolve(column_ctx);
        EXEC_ASSERT(condition->type() == Type::Bool,
                    "Тип выражения `on`(" + type_to_str(condition->type()) + ") не является bool");

        cartesian_product_on_condition(res, last_table, cur_table, std::move(condition));
    }
    return {res, column_ctx};
}
#include <sstream>
#include <vector>
#include <ranges>
#include "../include/select.h"
#include "../include/token.h"

using namespace std::string_literals;

namespace database {

    template<typename T>
    requires(std::convertible_to<T, value_t>)
    static std::shared_ptr<IColumn>
    _impl_fill(const std::vector<int> &valid, std::unique_ptr<Operation> val, const std::string &name) {
        std::vector<T> vals;
        vals.reserve(valid.size());
        int ind = 0;
        for (auto i: valid) {
            vals[ind++] = std::move(get<T>(val->eval(i)));
        }
        auto temp = std::make_shared<Column<T>>(val->type(), name);
        temp->set_rows(std::move(vals));
        return temp;
    }

    static std::shared_ptr<IColumn>
    fill(const std::vector<int> &valid, std::unique_ptr<Operation> val, const std::string &name) {
        if (val->type() == Type::Integer) {
            return _impl_fill<int>(valid, std::move(val), name);
        } else if (val->type() == Type::Boolean) {
            return _impl_fill<bool>(valid, std::move(val), name);
        } else if (val->type() == Type::String) {
            return _impl_fill<std::string>(valid, std::move(val), name);
        } else if (val->type() == Type::Bytes) {
            return _impl_fill<std::vector<bool>>(valid, std::move(val), name);
        }
        throw execution_error("Непредвиденная ошибка");
    }


    std::shared_ptr<Table> Select::parse_and_execute(const std::string &str, TableContext &ctx) const {
        std::string_view view(str);
        tokenize::skip_spaces(view);
        std::string temp;
        std::stringstream(std::string(view)) >> temp;
        if (to_lower(temp) != "select") {
            throw syntax_error("Ожидалось ключевое слово `select`, найдено: " + temp);
        }
        view.remove_prefix("select"s.size());

        auto cols_other = tokenize::Parser::clear_parse(std::string(view), "from", true);
        if (cols_other.size() != 2) {
            throw syntax_error("Ключевое слово `from` должно единожды встречаться в запросе");
        }
        std::string cols = cols_other[0];

        auto table_other = tokenize::Parser::clear_parse(cols_other[1], "where", true);
        if (table_other.size() != 2) {
            throw syntax_error("Ключевое слово `where` должно единожды встречаться в запросе");
        }
        std::string tables = table_other[0], cond = table_other[1];

        auto [table, column_ctx] = _resolve_table_expr(tables, ctx);

        auto columns = _resolve_column_expr(cols, column_ctx);

        auto condition = build_execution_tree_from_expression(cols, column_ctx);

        if (condition->type() != Type::Boolean) {
            throw execution_error("Тип выражения `where` не является bool");
        }

        auto table_cols = table->get_columns();
        std::vector<int> valid;
        int row_cnt = (table_cols.empty() ? 0 : table_cols[0]->size());
        for (int i = 0; i < row_cnt; ++i) {
            if (get<bool>(condition->eval(i))) {
                valid.push_back(i);
            }
        }

        auto result_table = std::make_shared<Table>("Table" + std::to_string(++ctx.number));
        for (auto &[col, name]: columns) {
            result_table->add_column(fill(valid, std::move(col), name));
        }
        return result_table;
    }

    std::vector<std::pair<std::unique_ptr<Operation>, std::string>>
    Select::_resolve_column_expr(const std::string &cols, ColumnContext &ctx) {
        std::vector<std::pair<std::unique_ptr<Operation>, std::string>> columns{};
        int col_num = 0;

        for (auto &full_column: tokenize::Parser::clear_parse(cols, ",", false)) {
            auto parts = tokenize::Parser::clear_parse(full_column, "as", true);
            if (parts.size() > 2) {
                throw syntax_error("Ключевое слово `as` должно не более одного раза встречаться в конце выражения");
            }

            auto col_expr = build_execution_tree_from_expression(parts[0], ctx);
            std::string name = "column " + std::to_string(++col_num);

            if (parts.size() == 1) {
                FieldOperation *column;
                if ((column = dynamic_cast<FieldOperation *>(col_expr.get()))) {
                    name = tokenize::Parser::clear_parse(column->name(), ".", false).back();
                }
            } else {
                auto view = std::string_view(parts[1]);
                tokenize::skip_spaces(view);
                name = tokenize::get_name(view);
                if (!tokenize::check_empty(view)) {
                    throw syntax_error("Непредвиденный литерал `" + std::string(view) + "`");
                }
            }

            columns.emplace_back(std::move(col_expr), name);

            std::cout << parts[0] << " | name = " << columns.back().second << " | value = "
                      << value_to_string(columns.back().first->eval(0), columns.back().first->type())
                      << " of type " << type_to_str(columns.back().first->type()) << std::endl;
        }
        return columns;
    }

    static void
    add_columns_to_context(std::shared_ptr<Table> &table, const std::string &name, size_t count, bool shorty,
                           ColumnContext &ctx) {
        auto cols = table->get_columns();
        for (auto col: std::views::counted(table->get_columns().begin(), count)) {
            auto full_name = name + "." + col->name();
            if (ctx.contains(full_name)) {
                throw syntax_error("Неоднозначно определен столбец `" + full_name + "`");
            }
            ctx[full_name] = col;
            if (shorty) {
                ctx.try_emplace(col->name(), col);
            }
        }
    }

    static std::shared_ptr<Table> select(const std::shared_ptr<Table> &table, std::unique_ptr<Operation> cond) {
        std::vector<int> row_nums;
        auto cols = table->get_columns();
        int rows = 0;
        if (!cols.empty()) {
            rows = cols[0]->size();
        }
        for (int i = 0; i < rows; ++i) {
            if (std::get<bool>(cond->eval(i))) {
                row_nums.push_back(i);
            }
        }
        for (auto &col: cols) {
            col->apply_changes(row_nums);
        }
    }

    static std::shared_ptr<Table>
    cartesian_product(const std::shared_ptr<Table> &table1, const std::shared_ptr<Table> &table2) {
        auto cols1 = table1->get_columns();
        auto cols2 = table2->get_columns();
        auto res = std::make_shared<Table>(table1->name() + "_" + table2->name());
        size_t sz1 = (cols1.empty() ? 0 : cols1[0]->size());
        size_t sz2 = (cols2.empty() ? 0 : cols2[0]->size());
        for (size_t i = 0; i < cols1.size(); i++) {
            res->add_column(cols1[i]->multicopy(sz2, false));
        }
        for (size_t i = 0; i < cols2.size(); i++) {
            res->add_column(cols2[i]->multicopy(sz1, true));
        }
        return res;
    }

    std::tuple<std::shared_ptr<Table>, ColumnContext>
    Select::_resolve_table_expr(const std::string &table_str, TableContext &ctx) {
        auto parts = tokenize::Parser::clear_parse(table_str, "join", true);

        std::vector<std::pair<std::string, int>> history;
        auto [table, alias] = get_table_from_expression(parts[0], ctx);
        table = table->copy();

        ColumnContext column_ctx;
        add_columns_to_context(table, alias, table->get_columns().size(), true, column_ctx);
        history.push_back({alias, table->get_columns().size()});

        for (auto it = parts.begin() + 1; it != parts.end(); ++it) {
            auto subparts = tokenize::Parser::clear_parse(*it, "on", true);
            if (subparts.size() != 2) {
                throw syntax_error("Некорректное использование ключевого слова `on`");
            }
            auto [cur_table, cur_alias] = get_table_from_expression(subparts[0], ctx);
            history.push_back({cur_alias, cur_table->get_columns().size()});

            table = cartesian_product(table, cur_table);

            column_ctx.clear();
            for (auto &[al, cnt]: history) {
                add_columns_to_context(table, al, cnt, false, column_ctx);
            }

            auto condition = build_execution_tree_from_expression(subparts[1], column_ctx);
            if (condition->type() != Type::Boolean) {
                throw execution_error("Тип выражения `on` не является bool");
            }
            table = select(table, std::move(condition));
        }
        return {table, column_ctx};
    }
}
#include "../include/insert.h"
#include "../include/token.h"

using namespace database;
using namespace std::string_literals;

std::vector<std::optional<value_t>> Insert::_parse_linear(std::string_view &view, const std::shared_ptr<Table> &table) {
    auto vals = tokenize::clear_parse(std::string(view), ",", false);
    auto cols = table->get_columns();

    SYNTAX_ASSERT(vals.size() == cols.size(), "Ожидался ввод " + std::to_string(cols.size()) + " значений, получено " +
                                              std::to_string(vals.size()));
    std::vector<std::optional<value_t>> res(vals.size());
    for (int i = 0; i < vals.size(); i++) {
        if (tokenize::check_empty(vals[i])) {
            continue;
        }
        auto val_view = std::string_view(vals[i]);
        if (cols[i]->type() == Type::Integer) {
            res[i] = tokenize::get_int(val_view);
        } else if (cols[i]->type() == database::Type::Boolean) {
            auto word = tokenize::get_word(val_view);\
            SYNTAX_ASSERT(word == "true" or word == "false", "Ожидался литерал типа bool, найден: " + word);
            res[i] = word == "true";
        } else if (cols[i]->type() == Type::Bytes) {
            res[i] = tokenize::get_bytes(val_view);
        } else if (cols[i]->type() == Type::String) {
            res[i] = tokenize::get_str(val_view);
        }
        SYNTAX_ASSERT(tokenize::check_empty(val_view), "Непредвиденный литерал: " + std::string(val_view));
    }
    return res;
}

std::vector<std::optional<value_t>>
Insert::_parse_by_names(std::string_view &view, const std::shared_ptr<Table> &table) {
    auto vals = tokenize::clear_parse(std::string(view), ",", false);
    auto cols = table->get_columns();

    std::map<std::string, int> name_to_index;
    for (int i = 0; i < cols.size(); ++i) {
        name_to_index.emplace(cols[i]->name(), i);
    }

    std::vector<std::optional<value_t>> res(vals.size());
    for (int i = 0; i < vals.size(); i++) {
        auto col_other = tokenize::clear_parse(vals[i], "=", false);
        SYNTAX_ASSERT(col_other.size() == 2,
                      "При использовании insert во второй форме ожидается ровно один символ `=` на столбец");
        auto col_view = std::string_view(col_other[0]);
        auto value_view = std::string_view(col_other[1]);
        auto col_name = tokenize::get_word(col_view);
        value_t value;
        SYNTAX_ASSERT(tokenize::check_empty(col_view), "Непредвиденный литерал: " + std::string(col_view));
        if (cols[i]->type() == Type::Integer) {
            value = tokenize::get_int(value_view);
        } else if (cols[i]->type() == database::Type::Boolean) {
            auto word = tokenize::get_word(value_view);\
            SYNTAX_ASSERT(word == "true" or word == "false", "Ожидался литерал типа bool, найден: " + word);
            value = static_cast<bool>(word == "true");
        } else if (cols[i]->type() == Type::Bytes) {
            value = tokenize::get_bytes(value_view);
        } else if (cols[i]->type() == Type::String) {
            value = tokenize::get_str(value_view);
        } else {
            SYNTAX_ASSERT(0, "Непредвиденная ошибка");
        }
        SYNTAX_ASSERT(tokenize::check_empty(value_view), "Непредвиденный литерал: " + std::string(value_view));
        EXEC_ASSERT(name_to_index.contains(col_name), "Неизвестный столбец: " + col_name);
        res[name_to_index[col_name]] = value;
    }
    return res;
}


std::vector<std::optional<value_t>>
Insert::_parse_value(const std::string &value_str, const std::shared_ptr<Table> &table) {
    auto value_view = std::string_view(value_str);
    tokenize::skip_spaces(value_view);
    tokenize::skip_spaces(value_view, true);
    SYNTAX_ASSERT(value_view.starts_with("("), "Ожидался символ `(`");
    SYNTAX_ASSERT(value_view.ends_with(")"), "Ожидался символ `)`");
    value_view.remove_prefix(1);
    value_view.remove_suffix(1);

    if (tokenize::clear_parse(value_str, "=", false).size() == 1) {
        return _parse_linear(value_view, table);
    }
    return _parse_by_names(value_view, table);
}

std::shared_ptr<Table> Insert::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string temp;

    SYNTAX_ASSERT(tokenize::to_lower(temp = tokenize::get_word(view)) == "insert",
                  "Ожидалось ключевое слово `insert`, найдено: " + temp);

    auto values_other = tokenize::clear_parse(std::string(view), "to", true);
    SYNTAX_ASSERT(values_other.size() == 2, "Ключевое слово `to` должно единожды встречаться в запросе");

    auto table_view = std::string_view(values_other[1]);
    auto name = tokenize::get_name(table_view);

    SYNTAX_ASSERT(tokenize::check_empty(table_view), "Непредвиденный литерал: " + std::string(table_view));
    EXEC_ASSERT(ctx.tables.contains(name), "Таблица `" + name + "` не найдена");\

    auto table = ctx.tables[name];

    table->add_row(_parse_value(values_other[0], table));

    return table;
}
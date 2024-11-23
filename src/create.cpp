#include "../include/create.h"
#include "../include/token.h"
#include "../include/column.h"

#include <ranges>


using namespace std::string_literals;
using namespace database;


std::tuple<bool, bool, bool> Create::_parse_attributes(std::string_view &attrs_view) {
    tokenize::skip_spaces(attrs_view);
    if (!attrs_view.starts_with("{")) {
        return {false, false, false};
    }
    attrs_view.remove_prefix(1);

    auto parts = tokenize::clear_parse(std::string(attrs_view), "}", false);
    SYNTAX_ASSERT(parts.size() == 2, "Невозможная комбинация `{` и `}`");

    auto attrs = tokenize::clear_parse(parts[0], ",", false);
    bool is_auto = false, is_key = false, is_unique = false;
    for (const auto &attr: attrs) {
        auto attr_view = std::string_view(attr);
        auto word = tokenize::to_lower(tokenize::get_word(attr_view));
        SYNTAX_ASSERT(word == "key" or word == "unique" or word == "autoincrement", "Неизвестный атрибут: " + word);
        SYNTAX_ASSERT(tokenize::check_empty(attr_view), "Непредвиденный литерал: " + std::string(attr_view));
        is_auto |= word == "autoincrement";
        is_key |= word == "key";
        is_unique |= word == "unique";
    }
    is_unique |= is_key;

    attrs_view.remove_prefix(parts[0].size() + 1);
    return {is_auto, is_key, is_unique};
}

std::shared_ptr<IColumn> Create::_parse_and_create_col(std::string_view &view) {
    bool is_def = false;
    auto [is_auto, is_key, is_unique] = _parse_attributes(view);

    auto name = tokenize::get_name(view);
    SYNTAX_ASSERT(!name.empty(), "Ожидалось имя столбца");

    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with(":"), "Ожидался символ `:`");
    view.remove_prefix(1);

    auto type = tokenize::to_lower(tokenize::get_name(view));

    tokenize::skip_spaces(view);
    is_def = view.starts_with("=");
    view.remove_prefix(is_def);

    std::shared_ptr<IColumn> col;
    if (type == "integer") {
        col = std::make_shared<Column<int>>(Type::Integer, name, is_unique, is_auto, is_def,
                                            is_def ? tokenize::get_int(view) : 0);
    } else if (!is_auto and type == "string") {
        col = std::make_shared<Column<std::string>>(Type::String, name, is_unique, is_auto, is_def,
                                                    is_def ? tokenize::get_str(view) : "");
    } else if (!is_auto and type == "bytes") {
        col = std::make_shared<Column<std::vector<bool>>>(Type::Bytes, name, is_unique, is_auto, is_def,
                                                          is_def ? tokenize::get_bytes(view) : std::vector<bool>());
    } else if (!is_auto and type == "bool") {
        bool default_value = false;
        if (is_def) {
            auto temp = tokenize::get_word(view);
            SYNTAX_ASSERT(temp == "true" or temp == "false", "Ожидался литерал типа bool, найден: " + temp);
            default_value = temp == "true";
        }
        col = std::make_shared<Column<bool>>(Type::Boolean, name, is_unique, is_auto, is_def, default_value);
    } else {
        EXEC_ASSERT(0, "Неизвестная комбинация типа и атрибутов");
    }

    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with(',') or view.empty(), "Непредвиденный литерал: " + std::string(view));

    view.remove_prefix(!view.empty());

    return col;
}

Table Create::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string word;

    SYNTAX_ASSERT(tokenize::to_lower(word = tokenize::get_word(view)) == "create",
                  "Ожидалось ключевое слово `create`, найдено: " + word);

    SYNTAX_ASSERT(tokenize::to_lower(word = tokenize::get_word(view)) == "table",
                  "Ожидалось ключевое слово `table`, найдено: " + word);

    auto name = tokenize::get_name(view);
    SYNTAX_ASSERT(!name.empty(), "Невозможное имя таблицы: " + std::string(view));
    EXEC_ASSERT(!ctx.tables.contains(name), "Таблица с именем " + name + " уже существует");

    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with("("), "Ожидалась `(`, найдено: " + std::string(1, view.front()));
    view.remove_prefix(1);

    tokenize::skip_spaces(view, true);
    SYNTAX_ASSERT(view.ends_with(")"), "Ожидалась `)`");
    view.remove_suffix(1);

    std::vector<std::shared_ptr<IColumn>> columns;
    do {
        columns.push_back(_parse_and_create_col(view));
    } while (!view.empty());

    return create(name, columns, ctx);
}

Table Create::create(const std::string &name, const std::vector<std::shared_ptr<IColumn>> &columns, TableContext &ctx) {
    auto table = Table(name);
    for (const auto &col: columns) {
        table.add_column(col);
    }
    ctx.tables.emplace(table.name(), table);
    return table;
}

#include "../../include/command/create.h"
#include "../../include/token.h"

#include <ranges>
#include <set>

using namespace std::string_literals;
using namespace tokenize;
using namespace database;


std::tuple<bool, bool, bool> Create::_parse_attributes(std::string_view &attrs_view) {
    skip_spaces(attrs_view);
    if (!attrs_view.starts_with("{")) {
        return {false, false, false};
    }

    auto parts = clear_parse(attrs_view, "}", false);
    SYNTAX_ASSERT(parts.size() == 2, "Невозможная комбинация `{` и `}`");

    auto temp = std::string_view(parts[0]);
    temp.remove_prefix(1);

    auto attrs = clear_parse(temp, ",", false);
    bool is_auto = false, is_key = false, is_unique = false;
    for (const auto &attr: attrs) {
        auto attr_view = std::string_view(attr);
        auto word = to_lower(get_word(attr_view));
        SYNTAX_ASSERT(word == "key" or word == "unique" or word == "autoincrement", "Неизвестный атрибут: " + word);
        SYNTAX_ASSERT(check_empty(attr_view), "Непредвиденный литерал: " + std::string(attr_view));
        is_auto |= word == "autoincrement";
        is_key |= word == "key";
        is_unique |= word == "unique";
    }
    is_unique |= is_key;

    attrs_view.remove_prefix(parts[0].size() + 1);
    return {is_auto, is_key, is_unique};
}

std::shared_ptr<IColumn> Create::_parse_and_create_col(const std::string &str) {
    auto view = std::string_view(str);
    bool is_def = false;
    auto [is_auto, is_key, is_unique] = _parse_attributes(view);

    auto name = get_name(view);
    SYNTAX_ASSERT(!name.empty(), "Ожидалось имя столбца");

    skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with(":"), "Ожидался символ `:`");
    view.remove_prefix(1);

    auto type = to_lower(get_name(view));

    skip_spaces(view);
    is_def = view.starts_with("=");
    view.remove_prefix(is_def);

    std::shared_ptr<IColumn> col;
    if (type == "integer") {
        col = std::make_shared<Column<int>>(Type::Integer, name, is_unique, is_auto, is_def,
                                            is_def ? get_int(view) : 0);
    } else if (!is_auto and type == "string") {
        col = std::make_shared<Column<std::string>>(Type::String, name, is_unique, is_auto, is_def,
                                                    is_def ? get_str(view) : "");
    } else if (!is_auto and type == "bytes") {
        col = std::make_shared<Column<std::vector<bool>>>(Type::Bytes, name, is_unique, is_auto, is_def,
                                                          is_def ? get_bytes(view) : std::vector<bool>());
    } else if (!is_auto and type == "bool") {
        bool default_value = false;
        if (is_def) {
            auto temp = get_word(view);
            SYNTAX_ASSERT(temp == "true" or temp == "false", "Ожидался литерал типа bool, найден: " + temp);
            default_value = temp == "true";
        }
        col = std::make_shared<Column<bool>>(Type::Bool, name, is_unique, is_auto, is_def, default_value);
    } else {
        EXEC_ASSERT(0, "Неизвестная комбинация типа и атрибутов");
    }

    skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with(',') or view.empty(), "Непредвиденный литерал: " + std::string(view));

    view.remove_prefix(!view.empty());

    return col;
}

Table Create::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string word;

    SYNTAX_ASSERT(to_lower(word = get_word(view)) == "create",
                  "Ожидалось ключевое слово `create`, найдено: " + word);

    SYNTAX_ASSERT(to_lower(word = get_word(view)) == "table",
                  "Ожидалось ключевое слово `table`, найдено: " + word);

    auto name = get_name(view);
    SYNTAX_ASSERT(!name.empty(), "Невозможное имя таблицы: " + std::string(view));
    EXEC_ASSERT(!ctx.tables.contains(name), "Таблица с именем " + name + " уже существует");

    skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with("("), "Ожидалась `(`, найдено: " + std::string(1, view.front()));
    view.remove_prefix(1);

    skip_spaces(view, true);
    SYNTAX_ASSERT(view.ends_with(")"), "Ожидалась `)`");
    view.remove_suffix(1);

    std::vector<std::shared_ptr<IColumn>> columns;
    auto columns_str = clear_parse(view, ",", false);
    for (const auto &col_str: columns_str) {
        columns.push_back(_parse_and_create_col(col_str));
    };

    return create(name, columns, ctx);
}

Table Create::create(const std::string &name, const std::vector<std::shared_ptr<IColumn>> &columns, TableContext &ctx) {
    auto table = Table(name);
    std::set<std::string> unique_names;
    for (const auto &col: columns) {
        table.add_column(col);
        EXEC_ASSERT(not unique_names.contains(col->name()), "Несколько столбцов с именем `" + col->name() + "`");
        unique_names.insert(col->name());
    }
    ctx.tables.emplace(table.name(), table);
    return table;
}

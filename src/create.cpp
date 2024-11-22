#include "../include/create.h"
#include "../include/token.h"

#include <ranges>


using namespace std::string_literals;
using namespace database;


std::tuple<bool, bool, bool> Create::_parse_attributes(const std::string &attrs_str) {
    auto attrs = tokenize::clear_parse(attrs_str, ",", false);
    bool is_unique = false, is_key = false, is_auto = false;
    for (const auto &attr: attrs) {
        auto v_attr = std::string_view(attr);
        auto word = tokenize::to_lower(tokenize::get_word(v_attr));
        SYNTAX_ASSERT(tokenize::check_empty(v_attr), "Непредвиденный литерал: " + std::string(v_attr));
        SYNTAX_ASSERT(word == "key" or word == "unique" or word == "autoincrement", "Неизвестный атрибут: " + word);
        is_unique |= word == "unique";
        is_key |= word == "key";
        is_auto |= word == "autoincrement";
    }
    is_unique |= is_key;
    return {is_unique, is_key, is_auto};
}

std::shared_ptr<IColumn> Create::_parse_and_create_col(std::string_view &view) {
    bool is_unique = false, is_key = false, is_def = false, is_auto = false;
    tokenize::skip_spaces(view);
    if (view.starts_with("{")) {
        view.remove_prefix(1);
        auto parts = tokenize::clear_parse(std::string(view), "}", false);
        SYNTAX_ASSERT(parts.size() == 2, "Непредвиденное появление `}`");
        std::tie(is_unique, is_key, is_auto) = _parse_attributes(parts[0]);
        view.remove_prefix(parts[0].size() + 1);
    }
    auto name = tokenize::get_name(view);
    SYNTAX_ASSERT(!name.empty(), "Ожидалось имя столбца");

    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with(":"), "Ожидался символ `:`");
    view.remove_prefix(1);

    auto type = tokenize::to_lower(tokenize::get_name(view));
    value_t def;

    tokenize::skip_spaces(view);
    is_def = view.starts_with("=");
    view.remove_prefix(is_def);

    std::shared_ptr<IColumn> col;
    if (type == "integer") {
        if (is_def) {
            def = tokenize::get_int(view);
        }
        col = std::make_shared<Column<int>>(Type::Integer, name, is_unique, is_auto, is_def, def);
    } else if (!is_auto and type == "string") {
        if (is_def) {
            def = tokenize::get_str(view);
        }
        col = std::make_shared<Column<std::string>>(Type::String, name, is_unique, is_auto, is_def, def);
    } else if (!is_auto and type == "bytes") {
        if (is_def) {
            def = tokenize::get_bytes(view);
        }
        col = std::make_shared<Column<std::vector<bool>>>(Type::Bytes, name, is_unique, is_auto, is_def, def);
    } else if (!is_auto and type == "bool") {
        if (is_def) {
            auto temp = tokenize::get_word(view);
            SYNTAX_ASSERT(temp == "true" or temp == "false", "Ожидался литерал типа bool, найден: " + temp);
            def = temp == "true";
        }
        col = std::make_shared<Column<bool>>(Type::Boolean, name, is_unique, is_auto, is_def, def);
    } else {
        EXEC_ASSERT(0, "Неизвестная комбинация типа и атрибутов");
    }

    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with(',') or view.empty(), "Непредвиденный литерал: " + std::string(view));

    view.remove_prefix(!view.empty());

    return col;
}

std::shared_ptr<Table> Create::parse_and_execute(const std::string &str, TableContext &ctx) const {
    std::string_view view(str);
    std::string temp;

    SYNTAX_ASSERT(tokenize::to_lower(temp = tokenize::get_word(view)) == "create",
                  "Ожидалось ключевое слово `create`, найдено: " + temp);

    SYNTAX_ASSERT(tokenize::to_lower(temp = tokenize::get_word(view)) == "table",
                  "Ожидалось ключевое слово `table`, найдено: " + temp);

    auto name = tokenize::get_name(view);
    SYNTAX_ASSERT(!name.empty(), "Невозможное имя таблицы: " + std::string(view));
    EXEC_ASSERT(!ctx.tables.contains(name), "Таблица с именем " + name + " уже существует");

    tokenize::skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with("("), "Ожидалась `(`, найдено: " + std::string(1, view.front()));
    view.remove_prefix(1);

    tokenize::skip_spaces(view, true);
    SYNTAX_ASSERT(view.ends_with(")"), "Ожидалась `)`");
    view.remove_suffix(1);

    auto table = std::make_shared<Table>(name);
    std::shared_ptr<IColumn> col;
    do {
        table->add_column(_parse_and_create_col(view));
    } while (!view.empty());

    table->check_valid();

    ctx.tables.emplace(table->name(), table);
    return table;
}

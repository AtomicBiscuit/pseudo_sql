
#include "../include/create.h"
#include "../include/token.h"

#include <sstream>
#include <ranges>

using namespace std::string_literals;

namespace database {

    static std::shared_ptr<IColumn> _create_col(std::string_view &view) {
        tokenize::skip_spaces(view);
        bool is_unique = false, is_def = false, is_auto = false;
        if (view.starts_with("{")) {
            view.remove_prefix(1);
            auto parts = tokenize::clear_parse(std::string(view), "}", false);
            if (parts.size() != 2) {
                throw syntax_error("Непредвиденное появление `}`");
            }
            auto attrs = tokenize::clear_parse(parts[0], ",", false);
            for (const auto &attr: attrs) {
                auto v_attr = std::string_view(attr);
                auto word = tokenize::to_lower(tokenize::get_word(v_attr));
                if (!tokenize::check_empty(v_attr)) {
                    throw syntax_error("Непредвиденный литерал: " + std::string(v_attr));
                }
                if (word == "key" or word == "unique" or word == "autoincrement") {
                    is_unique |= word == "unique";
                    is_unique |= word == "key";  // TODO key
                    is_auto |= word == "autoincrement";
                } else {
                    throw syntax_error("Неизвестный атрибут: " + word);
                }
            }
            view.remove_prefix(parts[0].size() + 1);
        }

        tokenize::skip_spaces(view);
        auto name = tokenize::get_name(view);
        if (name.empty()) {
            throw syntax_error("Ожидалось имя столбца");
        }

        tokenize::skip_spaces(view);
        if (!view.starts_with(":")) {
            throw syntax_error("Ожидался символ `:`");
        }
        view.remove_prefix(1);

        tokenize::skip_spaces(view);
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
                def = temp == "true" ? true :
                      temp == "false" ? false :
                      throw execution_error("Ожидался литерал типа bool, найден: " + temp);
            }
            col = std::make_shared<Column<bool>>(Type::Boolean, name, is_unique, is_auto, is_def, def);
        } else {
            throw execution_error("Неизвестная комбинация типа и атрибутов");
        }

        tokenize::skip_spaces(view);
        if (!view.starts_with(',') and !view.empty()) {
            throw syntax_error("Непредвиденный литерал: " + std::string(view));
        }
        if (!view.empty()) {
            view.remove_prefix(1);
        }
        return col;
    }

    std::shared_ptr<Table> Create::parse_and_execute(const std::string &str, TableContext &ctx) const {
        std::string_view view(str);
        std::string temp;

        tokenize::skip_spaces(view);
        if (tokenize::to_lower(temp = tokenize::get_word(view)) != "create") {
            throw syntax_error("Ожидалось ключевое слово `create`, найдено: " + temp);
        }

        tokenize::skip_spaces(view);
        if (tokenize::to_lower(temp = tokenize::get_word(view)) != "table") {
            throw syntax_error("Ожидалось ключевое слово `table`, найдено: " + temp);
        }

        tokenize::skip_spaces(view);
        auto name = tokenize::get_name(view);

        if (name.empty()) {
            throw execution_error("Невозможное имя таблицы: " + std::string(view));
        }
        if (ctx.tables.contains(name)) {
            throw execution_error("Таблица с именем " + name + " уже существует");
        }
        tokenize::skip_spaces(view);
        if (!view.starts_with("(")) {
            throw execution_error("Ожидалась `(`, найдено: " + std::string(1, view.front()));
        }
        view.remove_prefix(1);
        tokenize::skip_spaces(view, true);
        if (!view.ends_with(")")) {
            throw execution_error("Ожидалась `)`");
        }
        view.remove_suffix(1);

        auto table = std::make_shared<Table>(name);
        std::shared_ptr<IColumn> col;
        do {
            table->add_column(_create_col(view));
        } while (!view.empty());

        table->check_valid();

        ctx.tables.emplace(table->name(), table);
        return table;
    }
}
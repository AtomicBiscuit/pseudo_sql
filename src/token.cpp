#include "../include/token.h"

#include <string>

using namespace database;

std::string tokenize::get_word(std::string_view &view) {
    skip_spaces(view);
    auto c = view.begin();
    auto st = view.begin();
    int cnt = 0;
    while (c != view.end() and !" \n\r\t "s.contains(*c)) {
        ++cnt;
        ++c;
    }
    auto temp = std::string(st, st + cnt);
    view.remove_prefix(cnt);
    return temp;
}

std::string tokenize::get_str(std::string_view &view) {
    skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with("\""), "Ожидался литерал типа string, найдено: " + std::string(view));
    int cnt = 1;
    auto c = view.begin();
    c++;
    while (c != view.end() and *c != '"') {
        if (*(c++) == '\\') {
            ++cnt;
            if ((++c) == view.end()) {
                break;
            }
        }
        ++cnt;
    }
    SYNTAX_ASSERT(c != view.end(), "Не найден конец строки");

    auto temp = std::string(view.begin() + 1, view.begin() + cnt);
    view.remove_prefix(cnt + 1);
    return temp;
}

std::vector<bool> tokenize::get_bytes(std::string_view &view) {
    skip_spaces(view);
    SYNTAX_ASSERT(view.starts_with("0x"), "Ожидался литерал типа bytes, найдено: " + std::string(view));
    int cnt = 2;
    std::vector<bool> b;
    auto c = view.begin() + 2;
    while (c != view.end() and isxdigit(static_cast<unsigned char>(*c))) {
        auto C = static_cast<unsigned char>(*c);
        int n = (isdigit(C) ? C - '0' : tolower(C) - 'a' + 10);
        b.push_back(n / 8);
        b.push_back(n / 4 % 2);
        b.push_back(n / 2 % 2);
        b.push_back(n % 2);
        ++cnt;
        ++c;
    }
    view.remove_prefix(cnt);
    return b;
}

int tokenize::get_int(std::string_view &view) {
    skip_spaces(view);
    SYNTAX_ASSERT(!view.empty() and isdigit(view[0]), "Ожидался литерал типа int32, найдено: " + std::string(view));
    int cnt = 0;
    int b = 0;
    auto c = view.begin();
    while (c != view.end() and isdigit(static_cast<unsigned char>(*c))) {
        b = 10 * b + *c - '0';
        ++c;
        ++cnt;
    }
    view.remove_prefix(cnt);
    return b;
}

std::string tokenize::get_full_name(std::string_view &view) {
    skip_spaces(view);
    if (view.empty() or !isalpha(view[0])) {
        return "";
    }
    int cnt = 0;
    auto c = view.begin();
    while (c != view.end() and (isalnum(*c) or *c == '.' or *c == '_')) {
        ++c;
        ++cnt;
    }
    auto temp = std::string(view.begin(), view.begin() + cnt);
    view.remove_prefix(cnt);
    return temp;
}

std::string tokenize::get_name(std::string_view &view) {
    skip_spaces(view);
    if (view.empty() or !isalpha(view[0])) {
        return "";
    }
    int cnt = 0;
    auto c = view.begin();
    while (c != view.end() and (isalnum(*c) or *c == '_')) {
        ++c;
        ++cnt;
    }
    auto temp = std::string(view.begin(), view.begin() + cnt);
    view.remove_prefix(cnt);
    return temp;
}

void tokenize::skip_spaces(std::string_view &view, bool reversed) {
    if (!reversed) {
        while (!view.empty() and " \n\r\t"s.contains(view.front())) {
            view.remove_prefix(1);
        }
        return;
    }
    while (!view.empty() and " \n\r\t"s.contains(view.back())) {
        view.remove_suffix(1);
    }
}

value_t tokenize::get_value(std::string_view &view, Type type) {
    value_t res;
    if (type == Type::Integer) {
        res = tokenize::get_int(view);
    } else if (type == database::Type::Boolean) {
        auto word = tokenize::get_word(view);
        SYNTAX_ASSERT(word == "true" or word == "false", "Ожидался литерал типа bool, найден: " + word);
        res = static_cast<bool>(word == "true");
    } else if (type == Type::Bytes) {
        res = tokenize::get_bytes(view);
    } else if (type == Type::String) {
        res = tokenize::get_str(view);
    }
    return res;
}

bool tokenize::check_empty(const std::string_view &view) {
    return std::ranges::all_of(view, [](char c) { return c == ' ' or c == '\n' or c == '\r' or c == '\t'; });
}


/// Делит str на слова по разделителю del, но только есть del не находится в тексте "...del..."
/// и находится на 0 уровне вложенности
/// \param str строка для деления
/// \param del разделитель
/// \param is_separated если true, то требуется чтобы разделить был отделён с двух сторон кем-то из " \n\r\t,"
/// \return Вектор найденных слов без разделителей
std::vector<std::string> tokenize::clear_parse(const std::string &str, const std::string &del, bool is_separated) {
    if (del.starts_with('\\') or del.starts_with('"') or del.starts_with('(')) {
        throw std::domain_error("clear_parse не может обработать разделитель: `" + del + "`");
    }
    const std::string sep = " \n\r\t, ";
    std::string_view view(str);
    bool is_last_sep = true;
    bool skip = false;
    int level = 0;

    std::vector<std::string> res;
    auto start = view.begin();
    while (!view.empty()) {
        if (view[0] == '\\') {
            view.remove_prefix(1 + (view.size() > 1));
        } else if (view[0] == '"') {
            view.remove_prefix(1);
            skip = !skip;
            is_last_sep = false;
        } else if (skip) {
            view.remove_prefix(1);
        } else {
            level += view[0] == '(' ? 1 : (view[0] == ')' ? -1 : 0);
            if (level == 0 and to_lower(std::string(view)).starts_with(del)) {
                if (is_separated and is_last_sep and (view.size() == del.size() or sep.contains(view[del.size()]))) {
                    is_last_sep = false;
                    res.emplace_back(start, view.begin());
                    view.remove_prefix(del.size());
                    start = view.begin();
                    continue;
                } else if (!is_separated) {
                    res.emplace_back(start, view.begin());
                    view.remove_prefix(del.size());
                    start = view.begin();
                    continue;
                }
            }
            if (level == 0 and is_separated)
                is_last_sep = sep.contains(view[0]);
            view.remove_prefix(1);
        }
    }
    res.emplace_back(start, view.begin());
    return res;
}

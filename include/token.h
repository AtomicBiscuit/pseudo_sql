#pragma once

#include "data/table.h"
#include "data/column.h"

#include <memory>
#include <map>
#include <vector>
#include <ranges>
#include <algorithm>
#include <list>
#include <string>
#include <utility>
#include <iostream>

using std::string_literals::operator ""s;

namespace tokenize {
    void skip_spaces(std::string_view &view, bool reversed = false);

    bool check_empty(const std::string_view &view);

    std::string get_word(std::string_view &);

    std::string get_str(std::string_view &);

    std::vector<bool> get_bytes(std::string_view &);

    int get_int(std::string_view &);

    std::string get_full_name(std::string_view &);

    std::string get_name(std::string_view &);

    database::value_t get_value(std::string_view &, database::Type);

    std::vector<std::string> clear_parse(const std::string_view &, const std::string &, bool);

    std::string to_lower(std::string s);

    std::string to_lower(std::string_view s);
}
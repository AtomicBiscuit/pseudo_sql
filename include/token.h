#pragma once

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

    std::string get_full_name(std::string_view &view);

    std::string get_name(std::string_view &view);

    std::vector<std::string> clear_parse(const std::string &, const std::string &, bool);


    inline std::string to_lower(std::string s) {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return tolower(c); });
        return s;
    }

    inline std::string to_lower(std::string_view s) {
        return to_lower(std::string(s));
    }

}